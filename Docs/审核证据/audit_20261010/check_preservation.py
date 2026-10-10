from pathlib import Path
import csv, hashlib, json, re, sys

root = Path.cwd()
audit = root / 'build/audit_20261010'
baseline = audit / 'baseline'
stage = sys.argv[1]
allowed = {'Core/Src/main.c', 'DMF407_FOC_HAL.ioc'}
if stage == 'safety':
    allowed |= {'Core/Src/gpio.c', 'CMakeLists.txt'}
changed = []
for original in baseline.rglob('*'):
    if not original.is_file():
        continue
    rel = original.relative_to(baseline).as_posix()
    current = root / rel
    assert current.is_file(), f'Missing baseline file: {rel}'
    a, b = original.read_bytes(), current.read_bytes()
    if a.replace(b'\r\n', b'\n') != b.replace(b'\r\n', b'\n'):
        changed.append(rel)
        if not rel.startswith('Docs/'):
            assert rel in allowed, f'Unexpected source change: {rel}'
print('Changed existing files (excluding line-ending-only changes):', changed)
with (audit / 'legacy-before.csv').open(encoding='utf-8-sig', newline='') as f:
    records = list(csv.DictReader(f))
for record in records:
    assert hashlib.sha256((root / record['Path']).read_bytes()).hexdigest().upper() == record['SHA256']
assert len(list((root / 'Legacy').rglob('*'))) == len(list((baseline / 'Legacy').rglob('*')))
print(f'Legacy byte hashes unchanged: {len(records)} files')
for name in ('user-settings-before.json', 'user-clangd-before.json'):
    records = json.loads((audit / name).read_text(encoding='utf-8-sig'))
    if isinstance(records, dict):
        records = [records]
    for record in records:
        assert hashlib.sha256(Path(record['Path']).read_bytes()).hexdigest().upper() == record['Hash']
print('Existing .clangd and .settings files unchanged')
def ioc_values(path):
    return dict(line.split('=', 1) for line in path.read_text(encoding='utf-8').splitlines() if '=' in line)
old, new = ioc_values(baseline / 'DMF407_FOC_HAL.ioc'), ioc_values(root / 'DMF407_FOC_HAL.ioc')
keys = {k for k in old.keys() | new.keys() if old.get(k) != new.get(k)}
expected = {'RCC.PLLQ', 'RCC.IPParameters', 'RCC.PLLQCLKFreq_Value', 'RCC.48MHZClocksFreq_Value'}
if stage == 'safety':
    expected |= {'PF10.PinState', 'PF10.GPIOParameters'}
assert keys <= expected, f'Unexpected ioc changes: {keys - expected}'
assert new['RCC.PLLQ'] == '7'
assert new['RCC.PLLQCLKFreq_Value'] == new['RCC.48MHZClocksFreq_Value'] == '48000000'
for k,v in {'HSE_VALUE':'8000000', 'PLLM':'8', 'PLLN':'336', 'SYSCLKFreq_VALUE':'168000000',
            'APB1TimFreq_Value':'84000000', 'APB2TimFreq_Value':'168000000'}.items():
    assert new['RCC.'+k] == old['RCC.'+k] == v
main = (root / 'Core/Src/main.c').read_text(encoding='utf-8')
assert 'RCC_OscInitStruct.PLL.PLLQ = 7;' in main
assert 'RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;' in main
order = [main.index(call+'();') for call in ['BSP_ADC_Start','BSP_Timer_Start','BSP_Encoder_Start','BSP_PWM_Start']]
assert order == sorted(order)
if stage == 'clock':
    assert main.replace('RCC_OscInitStruct.PLL.PLLQ = 7;', 'RCC_OscInitStruct.PLL.PLLQ = 4;') == (baseline/'Core/Src/main.c').read_text(encoding='utf-8')
else:
    assert new.get('PF10.PinState', 'GPIO_PIN_RESET') == 'GPIO_PIN_RESET'
    gpio = (root / 'Core/Src/gpio.c').read_text(encoding='utf-8')
    write = 'HAL_GPIO_WritePin(PM1_CTRL_SD_GPIO_Port, PM1_CTRL_SD_Pin, GPIO_PIN_RESET);'
    assert gpio.index(write) < gpio.index('HAL_GPIO_Init(PM1_CTRL_SD_GPIO_Port, &GPIO_InitStruct);')
    assert main.index('BSP_MotorEnable_ForceDisable();') < order[0]
print('IOC changed keys:', sorted(keys))
print('Clock tree, startup order and stage-specific source checks: PASS')
