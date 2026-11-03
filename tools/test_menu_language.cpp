// Idioma do menu dev: chave salva, consulta da tabela e textos que faltam.
#include <cstdio>
#include <cstring>
#include <set>
#include <string_view>
#include "../src/common/menu_language.h"

static int failures=0;
static void Check(bool ok,const char* what) {
    if(!ok) { std::printf("FALHOU: %s\n",what);++failures; }
}

int main() {
    using namespace dev;
    // 1-3: chave do settings.toml
    Check(MenuLanguageIsEnglish("en"),"1 en e ingles");
    Check(!MenuLanguageIsEnglish("pt") && !MenuLanguageIsEnglish("") && !MenuLanguageIsEnglish("EN"),"2 o resto e portugues");
    Check(std::strcmp(MenuLanguageKey(true),"en")==0 && std::strcmp(MenuLanguageKey(false),"pt")==0,"3 chave gravada");
    // 4: portugues devolve o proprio ponteiro
    const char* label="Salvar opcoes";
    Check(MenuText(label,false)==label,"4 portugues sem consulta");
    // 5: ingles conhecido
    Check(std::strcmp(MenuText("Salvar opcoes",true),"Save settings")==0,"5 ingles da tabela");
    // 6: texto sem traducao continua aparecendo
    const char* unknown="Texto que nao existe na tabela";
    Check(MenuText(unknown,true)==unknown,"6 sem traducao volta o original");
    Check(MenuText(nullptr,true)==nullptr,"7 nulo seguro");
    // 8: chaves unicas (uma chave repetida faria a segunda traducao sumir em silencio)
    std::set<std::string_view> keys;
    for(const auto& [key,value]:kMenuStringsEn) {
        Check(keys.insert(key).second,"8 chave repetida");
        Check(value && *value,"9 ingles vazio");
    }
    // 10: rotulos de aba e o formato com especificadores
    Check(std::strcmp(MenuText("Jogabilidade",true),"Gameplay")==0,"10 aba");
    Check(std::strcmp(MenuText("Mapa %d  /  Sala %d",true),"Map %d  /  Room %d")==0,"11 formato preservado");
    if(failures) return 1;
    std::printf("Menu language: key, lookup, fallback, unique keys and formats passed\n");
    return 0;
}
