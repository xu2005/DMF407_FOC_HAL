from pathlib import Path
import hashlib, json, re

root = Path.cwd()
mapping = root / 'Docs/V1_Legacy_Mapping.md'
content = mapping.read_text(encoding='utf-8')
content, count = re.subn(r'\n\n(?=\| \*\*D-19\*\*)', '\n', content)
assert count == 1
row = ('| **D-20** | Legacy time1.c 初始化后将 GPIOF pin10 置 High | '
       '早于自检授权，R-03 / SF-02 安全风险 | CubeMX 默认 Low；'
       'MX_GPIO_Init 先 RESET 后配置模式，新增 ForceDisable 在 BSP 启动前重复禁能 | '
       '**是（有意安全偏离，仅软件 L1/L2）** | 用户最新 B 裁决；PD6010B_V1.0 p1；'
       '两处可达写入均 RESET；无置高 API，实板极性/复位高阻/BKIN 等未验收，风险不关闭。见本轮验收 §3 |\n')
lines = content.splitlines(keepends=True)
indexes = [i for i, line in enumerate(lines) if line.startswith('| **D-19**')]
assert len(indexes) == 1
lines.insert(indexes[0] + 1, row)
mapping.write_text(''.join(lines), encoding='utf-8')

audit = root / 'build/audit_20261010'
records = json.loads((audit/'pf10-artifacts.json').read_text(encoding='utf-8-sig'))
for record in records:
    for folder in [root/'build/Debug', audit/'pf10-artifacts']:
        data = (folder/record['Name']).read_bytes()
        assert len(data) == record['Bytes']
        assert hashlib.sha256(data).hexdigest().upper() == record['SHA256']
print('Final build and archived ELF/MAP/BIN/HEX hashes match recorded evidence')
disassembly = (audit/'final-disassembly.txt').read_text(encoding='utf-8-sig')
calls = re.findall(r'^.*\bbl\s+[^\n]*<HAL_GPIO_WritePin>.*$', disassembly, re.M)
assert len(calls) == 2
assert not re.search(r'<HAL_GPIO_TogglePin>:', disassembly)
assert not re.search(r'<SystemInit_ExtMemCtl>:', disassembly)
print('ELF: exactly two GPIO write calls; no Toggle or external-memory init function')
print('\n'.join(calls))
