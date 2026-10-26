"""Gera src/common/menu_search_index.h: rotulos das opcoes do menu dev por pagina, para a barra de filtro.

Le dev_menu.cpp, acha as paginas no switch (`case N:Funcao(`), junta os rotulos literais de cada funcao de
pagina e das funcoes do arquivo que ela chama. Uso: `python tools/menu_search_index.py` (grava) ou
`--check` (falha se o header estiver desatualizado).
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MENU = ROOT / "src/common/dev_menu.cpp"
HEADER = ROOT / "src/common/menu_search_index.h"

DEFINITION = re.compile(r"^(?:static\s+)?(?:void|bool|int)\s+(\w+)\([^)]*\)\s*\{", re.M)
PAGE = re.compile(r"case\s+(\d+)\s*:\s*(\w+)\(")
LABEL = re.compile(
    r"\b(?:Toggle|Slider|Heading|ImGui::(?:Checkbox|Button|SliderFloat|SliderInt|Combo|SeparatorText|Selectable))"
    r"\(\s*(?:Tr\(\s*)?\"((?:[^\"\\]|\\.)*)\"")
CALL = re.compile(r"\b(\w+)\(")


def bodies(source):
    """Nome da funcao -> corpo entre chaves (ignora chaves dentro de strings e caracteres)."""
    found = {}
    for match in DEFINITION.finditer(source):
        depth, i, quote = 0, match.end() - 1, None
        while i < len(source):
            c = source[i]
            if quote:
                if c == "\\":
                    i += 1
                elif c == quote:
                    quote = None
            elif c in "\"'":
                quote = c
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        found.setdefault(match.group(1), source[match.end():i])
    return found


def labels(name, functions, seen):
    if name in seen or name not in functions:
        return []
    seen.add(name)
    body = functions[name]
    out = []
    for label in LABEL.findall(body):
        label = label.split("##", 1)[0].strip()
        if label and "%" not in label:
            out.append(label)
    for callee in CALL.findall(body):
        out += labels(callee, functions, seen)
    return out


def entries(source):
    functions = bodies(source)
    result = []
    for page, function in PAGE.findall(source):
        seen_labels = set()
        for label in labels(function, functions, set()):
            if label not in seen_labels:
                seen_labels.add(label)
                result.append((int(page), label))
    return result


def render(source):
    lines = [
        "#pragma once",
        "// Gerado por tools/menu_search_index.py a partir de dev_menu.cpp. Nao edite a mao.",
        "namespace dev {",
        "struct MenuSearchEntry { int page; const char* label; };",
        "inline constexpr MenuSearchEntry kMenuSearchIndex[]{",
    ]
    lines += [f'    {{{page}, "{label}"}},' for page, label in entries(source)]
    lines += ["};", "}", ""]
    return "\n".join(lines)


def main(argv):
    text = render(MENU.read_text(encoding="utf-8"))
    if "--check" in argv:
        current = HEADER.read_text(encoding="utf-8") if HEADER.exists() else ""
        if current != text:
            print("menu_search_index.h desatualizado: rode python tools/menu_search_index.py")
            return 1
        return 0
    HEADER.write_text(text, encoding="utf-8", newline="\n")
    print(f"{HEADER.name}: {text.count(chr(10)) - 7} rotulos")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
