import unittest

import menu_strings as strings


class MenuStrings(unittest.TestCase):
    def test_table_covers_menu_source(self):
        menu = strings.MENU.read_text(encoding="utf-8")
        others = [p.read_text(encoding="utf-8") for p in strings.OTHER if p.exists()]
        pairs = strings.table(strings.TABLE.read_text(encoding="utf-8"))
        self.assertEqual(strings.problems(menu, others, pairs), [], "rode python tools/menu_strings.py")

    def test_collects_every_kind_of_label(self):
        menu = (
            'Heading("Titulo", "Subtitulo");\nToggle("Ligar algo",flag);\nSlider("Barra",v,0,1,"%.2f");\n'
            'ImGui::Text("%s",Tr(ok ? "Sim; feito" : "Nao"));\nconstexpr Tab t[]{{"Aba",{{"Pagina",3}},1}};\n'
            'action_status=x ? "Pronto." : "Falhou; tente";\nImGui::Button("##oculto");\nTr("%d");\n')
        other = ('NotifyToast("Aviso fixo");\nNotifyToast("Print: "+path);\n'
                 'report_status = a ? "Um; dois" : "Tres";\n{"Ataque (X)", VK_SPACE, 0}, {"Frente",\'W\',20},\n')
        self.assertEqual(strings.keys(menu, [other]), {
            "Titulo", "Subtitulo", "Ligar algo", "Barra", "Sim; feito", "Nao", "Aba", "Pagina", "Pronto.",
            "Falhou; tente", "Aviso fixo", "Um; dois", "Tres", "Ataque (X)", "Frente"})

    def test_reports_missing_unused_and_format_mismatch(self):
        menu = 'Toggle("Ligar",a);\nImGui::Text(Tr("Mapa %d"),m);\nImGui::Text("%s",Tr("100% feito"));\n'
        pairs = {"Mapa %d": "Map %s", "100% feito": "100% done", "Antigo": "Old"}
        self.assertEqual(strings.problems(menu, [], pairs), [
            "falta traducao: 'Ligar'", "chave sem uso no codigo: 'Antigo'",
            "formato diferente: 'Mapa %d' -> 'Map %s'"])


if __name__ == "__main__":
    unittest.main()
