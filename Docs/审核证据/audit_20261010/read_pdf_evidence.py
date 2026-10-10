from pathlib import Path
from pypdf import PdfReader

out = Path(__file__).parent
schematic = Path(r'C:\Users\xuhengyi\Desktop\item\课设\综合项目实训\综合项目实训\ATK-PD6010B_V1.0原理图.pdf')
text = PdfReader(schematic).pages[0].extract_text()
(out / 'pd6010b-page1.txt').write_text(text, encoding='utf-8')
lines = text.splitlines()
for i, line in enumerate(lines):
    if any(k in line for k in ('CTRL_SD=', '焊接R70', '不焊接R70', 'H桥无效', '清除锁存')):
        print('\n'.join(lines[max(0,i-2):i+5]))
manual = PdfReader(r'D:\STM32CubeMX\help\UM1718.pdf')
matches = []
for i, page in enumerate(manual.pages):
    content = page.extract_text() or ''
    if any(k in content.lower() for k in ('command line', 'command-line', 'script mode', 'setconfig', 'setparam', 'clockconfig')):
        matches.append((i+1, content))
(out / 'cubemx-cli-manual.txt').write_text('\n\n'.join(f'PDF page {i}\n{s}' for i,s in matches), encoding='utf-8')
print('CubeMX manual matching pages:', [i for i,s in matches])
