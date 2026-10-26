import unittest

import menu_search_index as index


class MenuSearchIndex(unittest.TestCase):
    def test_header_matches_menu_source(self):
        source = index.MENU.read_text(encoding="utf-8")
        self.assertEqual(index.HEADER.read_text(encoding="utf-8"), index.render(source),
                         "rode python tools/menu_search_index.py")

    def test_collects_page_and_called_helpers(self):
        source = (
            'void Helper() {\n    Toggle("Dentro do ajudante",flag);\n}\n'
            'void PageA() {\n    Heading("Pagina A", "sub");\n    Slider("Barra ##id",v,0,1,"%.2f");\n'
            '    ImGui::Button("##oculto");\n    ImGui::Text("{ nao e chave");\n    Helper();\n}\n'
            'void Draw() {\n    switch(page) {case 3:PageA();break;}\n}\n')
        self.assertEqual(index.entries(source),
                         [(3, "Pagina A"), (3, "Barra"), (3, "Dentro do ajudante")])


if __name__ == "__main__":
    unittest.main()
