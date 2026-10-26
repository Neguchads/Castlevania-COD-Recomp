"""Confere a tabela de traducao do menu dev (src/common/menu_strings_en.h).

O texto em portugues do codigo e a chave; a tabela traz o ingles. Este script junta todo texto que o menu
pode mostrar traduzido (argumentos de Tr(), rotulos de Toggle/Slider/Heading, abas, presets, avisos e
acoes do teclado) e falha se faltar traducao, se sobrar chave que o codigo nao usa ou se o ingles trocar
os especificadores de formato (%s, %d, %.2f...), que derrubariam o printf.

Uso: `python tools/menu_strings.py` (confere) ou `--missing` (imprime linhas prontas para as chaves que faltam).
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MENU = ROOT / "src/common/dev_menu.cpp"
TABLE = ROOT / "src/common/menu_strings_en.h"
# Avisos e estados mostrados pelo menu que nascem em outros arquivos.
OTHER = [ROOT / "src/common/dev_runtime.cpp", ROOT / "src/common/dev_controller.cpp",
         ROOT / "src/game/language.cpp", ROOT / "src/common/d3d12_runtime.inc", ROOT / "src/common/main_runtime.inc"]

LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"(?!\s*\+)')
# especificadores do printf usados no menu; "100% da" fora de formato nao conta
SPEC = re.compile(r"%%|%[-+#0]*\d*(?:\.\d+)?[diuxXfFeEgGcsp]")
# atribuicao ate o ";" que fecha a instrucao (o ";" dentro de string nao conta)
STATEMENT = r'{}\s*=\s*((?:"(?:[^"\\]|\\.)*"|[^;"])*);'
PAIR = re.compile(r'^\s*\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\},?\s*$', re.M)


def call_args(source, name):
    """Texto entre os parenteses de cada chamada `name(` (parenteses balanceados, ignorando strings)."""
    for match in re.finditer(r"\b" + re.escape(name) + r"\(", source):
        depth, i, quote = 1, match.end(), None
        while i < len(source) and depth:
            c = source[i]
            if quote:
                if c == "\\":
                    i += 1
                elif c == quote:
                    quote = None
            elif c in "\"'":
                quote = c
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            i += 1
        yield source[match.end():i - 1]


def literals(text):
    return [m.group(1) for m in LITERAL.finditer(text)]


def first_literal(text):
    found = LITERAL.match(text.strip())
    return [found.group(1)] if found else []


def keys(menu, others):
    found = set()
    for args in call_args(menu, "Tr"):
        found.update(literals(args))
    for name in ("Toggle", "Slider"):
        for args in call_args(menu, name):
            found.update(first_literal(args))
    for args in call_args(menu, "Heading"):
        found.update(literals(args)[:2])
    # abas, paginas e presets: {"Nome",numero...}
    found.update(re.findall(r'\{"([^"]+)",\s*[\d.{]', menu))
    for line in re.findall(STATEMENT.format("action_status"), menu):
        found.update(literals(line))
    for source in [menu, *others]:
        for args in call_args(source, "NotifyToast"):
            found.update(literals(args))
        for line in re.findall(STATEMENT.format("report_status"), source):
            found.update(literals(line))
        # acoes do teclado: {"Nome", VK_..., n} ou {"Nome",'W',n}
        found.update(re.findall(r'\{"([^"]+)",\s*(?:VK_\w+|\'.\')\s*,\s*\d+\}', source))
    # sem letras (formatos, numeros) e IDs ocultos nao precisam de traducao
    return {k for k in found if re.search(r"[A-Za-zÀ-ú]{2,}", SPEC.sub("", k)) and not k.startswith("##")}


def table(text):
    return dict(PAIR.findall(text))


def problems(menu, others, pairs):
    wanted = keys(menu, others)
    errors = [f"falta traducao: {k!r}" for k in sorted(wanted - pairs.keys())]
    errors += [f"chave sem uso no codigo: {k!r}" for k in sorted(pairs.keys() - wanted)]
    for pt, en in pairs.items():
        if not en.strip():
            errors.append(f"ingles vazio: {pt!r}")
        if SPEC.findall(pt) != SPEC.findall(en):
            errors.append(f"formato diferente: {pt!r} -> {en!r}")
    return errors


def main():
    menu = MENU.read_text(encoding="utf-8")
    others = [p.read_text(encoding="utf-8") for p in OTHER if p.exists()]
    pairs = table(TABLE.read_text(encoding="utf-8")) if TABLE.exists() else {}
    if "--missing" in sys.argv:
        for k in sorted(keys(menu, others) - pairs.keys()):
            print(f'    {{"{k}", ""}},')
        return 0
    errors = problems(menu, others, pairs)
    for error in errors:
        print(error)
    print(f"{len(pairs)} traducoes, {len(errors)} problemas")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
