#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include "app/app.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "area_names.h"
#include "dev_runtime.h"
#include "menu_input.h"
#include "menu_language.h"
#include "menu_nav.h"
#include "game_cheat_rules.h"
#include "drop_rules.h"
#include "enemy_editor.h"
#include "menu_search.h"
#include "menu_search_index.h"
#include "presentation_math.h"
#include "report_gallery.h"
#include "translation_pack_rules.h"

namespace dev {
namespace {
int page = 0;
MenuNav nav;
// Texto do menu no idioma escolhido (rodape ou Menu > Aparencia); o portugues do codigo e a chave.
const char* Tr(const char* pt) { return MenuText(pt,menu_english.load()); }
void SetMenuEnglish(bool english) {
    menu_english.store(english);
    LOGR("dev: menu language %s",MenuLanguageKey(english));
    app::Save();
}
struct MenuTitleEntry {const char* label;int page;};
struct MenuTabEntry {const char* label;MenuTitleEntry titles[12];int count;};
// Tudo que muda a partida fica em Jogabilidade (inimigos, drops e cheats inclusos).
constexpr MenuTabEntry kTabs[]{
    {"Jogabilidade",{{"Jogo e minimapa",0},{"Dificuldade",16},{"Inimigos: todos",18},{"Inimigos: aleatorio",19},
        {"Inimigos: por tipo",17},{"Drops e itens",20},{"Cheats",6},{"HUD e pets",10},{"Camera",11},{"Saves e sessao",8}},10},
    {"Graficos",{{"Graficos",2}},1},
    {"Audio",{{"Audio",3}},1},
    {"Controles",{{"Controle",15},{"Teclado e mouse",4}},2},
    {"Mods",{{"Mods e texturas",5}},1},
    {"Suporte",{{"Relatar bug",1},{"Capturas salvas",13},{"Logs e travamentos",14},{"Diagnostico",12}},4},
    {"Menu",{{"Aparencia",7},{"Perfis",9}},2}};
// Abre direto a pagina pedida (ex.: captura de bug leva ao relato).
void ShowPage(int target) {
    for(int tab=0;tab<int(std::size(kTabs));++tab)
        for(int title=0;title<kTabs[tab].count;++title)
            if(kTabs[tab].titles[title].page==target) { nav.tab=tab;EnterMenuOptions(nav,title);page=target;return; }
}
// Caminho "Aba > Pagina" mostrado nos resultados da busca.
std::string PagePath(int target) {
    for(const auto& tab:kTabs)
        for(int title=0;title<tab.count;++title)
            if(tab.titles[title].page==target) return std::string(Tr(tab.label))+" > "+Tr(tab.titles[title].label);
    return {};
}
// Barra de filtro: texto digitado e a opcao escolhida nos resultados (a pagina rola ate ela).
char search_text[64]{};
bool search_keys=false,search_keys_focus=false;   // teclado na tela aberto; foca a primeira tecla ao abrir
std::string jump_label;
void ScrollIfJumped(const char* label) {
    if(jump_label.empty() || jump_label!=label) return;
    ImGui::SetScrollHereY(.3f);
    jump_label.clear();
}
char title[160] = {}, description[8192] = {};
char profile_name[65] = "Meu perfil";
ImVec4 gold{.88f,.76f,.50f,1}, crimson{.75f,.19f,.25f,1};
const ImVec4 muted_text{.77f,.70f,.61f,1};
ImFont* section_font = nullptr;
ImFont* brand_font = nullptr;
ImVec2 MenuSize(float width,float height) {
    const float scale=ImGui::GetIO().FontGlobalScale;
    return {width>0 ? width*scale : width,height>0 ? height*scale : height};
}
void DrawToast() {
    const std::string message=Tr(ToastMessage().c_str());
    if(message.empty())return;
    const auto& io=ImGui::GetIO();
    const float scale=std::clamp(std::min(io.DisplaySize.x/1920.0f,io.DisplaySize.y/1080.0f),.65f,1.5f);
    const float max_width=std::max(160.0f,std::min(io.DisplaySize.x-48.0f,760.0f*scale)-32.0f*scale);
    const ImVec2 text_size=ImGui::CalcTextSize(message.c_str(),nullptr,false,max_width);
    const ImVec2 padding{18.0f*scale,12.0f*scale};
    const ImVec2 size{text_size.x+padding.x*2,text_size.y+padding.y*2};
    const ImVec2 position{std::max(12.0f,(io.DisplaySize.x-size.x)*.5f),18.0f*scale};
    const ImVec2 end{position.x+size.x,position.y+size.y};
    auto* draw=ImGui::GetForegroundDrawList();
    draw->AddRectFilled(position,end,IM_COL32(12,9,12,238),7.0f*scale);
    draw->AddRect(position,end,IM_COL32(159,130,79,255),7.0f*scale,0,1.5f*scale);
    draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{position.x+padding.x,position.y+padding.y},
        IM_COL32(245,222,173,255),message.c_str(),nullptr,max_width);
}
// Contador por cima do jogo (Graficos > Mostrar FPS), visivel com o menu fechado.
void DrawFps(double fps) {
    if(!show_fps.load())return;
    const auto& io=ImGui::GetIO();
    const float scale=std::clamp(std::min(io.DisplaySize.x/1920.0f,io.DisplaySize.y/1080.0f),.65f,1.5f);
    char text[48];
    std::snprintf(text,sizeof(text),"%.0f FPS  %.1f ms",fps,fps>0 ? 1000.0/fps : 0.0);
    const float font=ImGui::GetFontSize()*1.1f;
    const ImVec2 size=ImGui::GetFont()->CalcTextSizeA(font,FLT_MAX,0,text);
    const ImVec2 padding{10*scale,6*scale};
    const int corner=std::clamp(fps_corner.load(),0,3);
    const float margin=12*scale;
    const ImVec2 position{corner&1 ? io.DisplaySize.x-size.x-padding.x*2-margin : margin,
                          corner&2 ? io.DisplaySize.y-size.y-padding.y*2-margin : margin};
    // verde a partir de 55, amarelo de 30 a 55, vermelho abaixo
    const ImU32 color=fps>=55 ? IM_COL32(120,230,120,255) : fps>=30 ? IM_COL32(240,210,90,255) : IM_COL32(240,90,90,255);
    auto* draw=ImGui::GetForegroundDrawList();
    draw->AddRectFilled(position,{position.x+size.x+padding.x*2,position.y+size.y+padding.y*2},IM_COL32(0,0,0,170),4*scale);
    draw->AddText(ImGui::GetFont(),font,{position.x+padding.x,position.y+padding.y},color,text);
}

void Heading(const char* name, const char* subtitle) {
    ImGui::PushFont(section_font);
    ImGui::TextColored(gold, "%s", Tr(name));
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text,muted_text);
    ImGui::TextWrapped("%s", Tr(subtitle));
    ImGui::PopStyleColor();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
}
bool Toggle(const char* label, std::atomic<bool>& setting) {
    ScrollIfJumped(label);
    bool value = setting.load();
    // ### deixa o ID do item igual nos dois idiomas
    char id[512];std::snprintf(id,sizeof(id),"%s###%s",Tr(label),label);
    if (!ImGui::Checkbox(id, &value)) return false;
    setting.store(value); return true;
}
bool Slider(const char* label, std::atomic<float>& setting, float minimum, float maximum, const char* format) {
    ScrollIfJumped(label);
    float value = setting.load();
    ImGui::PushID(label);
    ImGui::TextUnformatted(Tr(label));
    ImGui::SetNextItemWidth(-1);
    const bool changed=ImGui::SliderFloat("##value", &value, minimum, maximum, format);
    ImGui::PopID();
    if (!changed) return false;
    setting.store(value); return true;
}
void GameHacks() {
    ImGui::BeginDisabled(!godmode_ready);
    if (Toggle("Godmode: proteger a vida do jogador",godmode))
        LOGR("dev: godmode %s",godmode.load() ? "enabled" : "disabled");
    ImGui::EndDisabled();
    if (!godmode_ready) ImGui::TextColored(crimson,"%s",Tr("A assinatura desta versao do jogo ainda nao foi confirmada."));
    else ImGui::TextColored(muted_text,Tr("Danos bloqueados nesta sessao: %u"),prevented_damage.load());
    ImGui::TextWrapped("%s",Tr("Preserva cura e outros personagens. Impactos e animacoes continuam; nao reverte morte ja iniciada."));
    ImGui::Spacing();
    float factor = speed.load();
    if(ImGui::SliderFloat("Speedhack",&factor,.5f,2.0f,"%.2fx")) SetSpeed(factor,false);
    if(ImGui::IsItemDeactivatedAfterEdit()) LOGR("dev: guest speed %.2fx (clock, waits, vblank, audio)",speed.load());
    const float factors[]{.5f,1,1.5f,2};
    const char* labels[]{"0.5x",Tr("Normal"),"1.5x","2x"};
    for(int i=0;i<4;i++) {
        if(i) ImGui::SameLine();
        if(ImGui::Button(labels[i],MenuSize(82,30))) SetSpeed(factors[i]);
    }
    ImGui::TextWrapped("%s",Tr("Escala os relogios do Xbox, timers, quadros e audio. O tom do som acompanha a velocidade. Vsync volta a valer em 1x."));
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::BeginDisabled(!godmode_ready);
    static const char* action_status="";
    if(ImGui::Button(Tr("Vida cheia"),MenuSize(150,30))) action_status=RefillPlayerHp() ? "Vida restaurada." : "Vida ja cheia ou jogador fora de partida.";
    ImGui::SameLine();
    if(ImGui::Button(Tr("MP cheio"),MenuSize(150,30))) action_status=RefillPlayerMp() ? "MP restaurado." : "MP ja cheio ou jogador fora de partida.";
    ImGui::SameLine();
    if(ImGui::Button(Tr("Ouro maximo"),MenuSize(150,30))) action_status=MaxPlayerGold() ? "Ouro em 9.999.999." : "Memoria do ouro indisponivel.";
    if(*action_status) ImGui::TextColored(muted_text,"%s",Tr(action_status));
    Slider("Ganho de ouro",gold_gain,kMinGainFactor,kMaxGainFactor,"%.1fx");
    Slider("Ganho de EXP",exp_gain,kMinGainFactor,kMaxGainFactor,"%.1fx");
    Toggle("Compras nao gastam ouro",free_shopping);
    Toggle("MP infinito",infinite_mp);
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s",Tr("Ouro e EXP passam pelo setter original do jogo (limite de 9.999.999 e nivel 99 mantidos). Vida cheia nao revive."));
    ImGui::Spacing();
    ImGui::TextColored(gold,"%s",Tr("Codigos de nome do jogo"));
    ImGui::TextWrapped("%s",Tr("Ao criar o save, use o nome @CRAZY (modo Crazy, mais dificil) ou @TREVOR (jogar com Trevor). Fonte: guias do jogo; em algumas versoes exige ter zerado. Nao testado nesta versao."));
    ImGui::Spacing();
    ImGui::TextColored(gold,"%s",Tr("Bonus do save de Lament of Innocence"));
    Toggle("Simular save de Lament of Innocence",lament_save);
    ImGui::TextWrapped("%s",Tr("No PS2, ter o save de Lament of Innocence no memory card libera a estatua Moai (vale 5000 de ouro). Ligue antes de comecar um jogo novo ou carregar; o bonus fica gravado no save quando voce salvar e nao sai desligando a opcao."));
}
void General() {
    Heading("Opcoes de jogo", "Minimapa, interface e ferramentas da partida.");
    Toggle("Mostrar minimapa",minimap_enabled);
    Toggle("Girar minimapa conforme a camera",minimap_rotating);
    ImGui::TextDisabled("%s",Tr("Menus originais: escala 100%. Reducao inteligente adiada."));
    int language=ptbr_enabled.load() ? 1 : 0;
    const char* languages[]{"English","Português (Brasil)"};
    if(ImGui::Combo(Tr("Idioma do jogo"),&language,languages,2)) SelectTextLanguage(language==1 ? 2 : 1);
    ImGui::TextDisabled("%s",Tr("Textos mudam na hora; imagens e vozes ja carregadas mudam ao reiniciar. Japones: Opcoes > Idioma no jogo."));
    Slider("Tamanho do minimapa",minimap_scale,.6f,1.6f,"%.2fx");
    Slider("Opacidade do minimapa",minimap_opacity,.2f,1.0f,"%.2f");
    Slider("Posicao horizontal do minimapa",minimap_x,0,1,"%.2f");
    Slider("Posicao vertical do minimapa",minimap_y,0,1,"%.2f");
    if(ImGui::Button(Tr("Minimapa no canto superior direito"),MenuSize(-1,30))) { minimap_x.store(1);minimap_y.store(0); }
    ImGui::TextDisabled("%s",Tr("0 = esquerda/topo, 1 = direita/base. Salvar opcoes grava a posicao."));
}
void DifficultyPage() {
    Heading("Dificuldade", "Multiplica o dano que o jogador e o Innocent Devil recebem.");
    struct Preset {const char* name;float taken,devil;};
    constexpr Preset presets[]{{"Facil",.5f,.5f},{"Normal",1,1},{"Dificil",1.5f,1.5f},{"Muito dificil",2,2},{"Pesadelo",3,3}};
    for(size_t i=0;i<std::size(presets);++i) {
        if(i) ImGui::SameLine();
        const bool current=damage_taken.load()==presets[i].taken && devil_damage_taken.load()==presets[i].devil;
        if(current) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        if(ImGui::Button(Tr(presets[i].name),MenuSize(0,30))) { damage_taken.store(presets[i].taken);devil_damage_taken.store(presets[i].devil); }
        if(current) ImGui::PopStyleColor();
    }
    Slider("Dano recebido pelo jogador",damage_taken,kMinDamageFactor,kMaxDamageFactor,"%.2fx");
    Slider("Dano recebido pelo Innocent Devil",devil_damage_taken,kMinDamageFactor,kMaxDamageFactor,"%.2fx");
    ImGui::TextWrapped("%s",Tr("Golpe que mata e mortes por roteiro continuam matando. Godmode, quando ligado, vale antes disto."));
    ImGui::TextColored(muted_text,"%s",Tr("Vida, tamanho, sorteio e elites dos inimigos: Jogabilidade > Inimigos."));
}
// Combinacoes prontas da aba Inimigos: global, sorteio por inimigo e elite juntos.
struct EnemyPreset {const char* name;float hp,scale;bool random;float scale_min,scale_max,hp_min,hp_max,elite;};
constexpr EnemyPreset kEnemyPresets[]{
    {"Original",1,1,false,.8f,1.3f,1,1.5f,0},
    {"Variados",1,1,true,.8f,1.3f,1,1.5f,0},
    {"Elites",1,1,true,1,1,1,1,15},
    {"Gigantes",3,2,false,.8f,1.3f,1,1.5f,0},
    {"Caotico",1,1,true,.5f,2.5f,.5f,3,10}};
bool PresetActive(const EnemyPreset& p) {
    if(enemy_hp.load()!=p.hp || enemy_scale.load()!=p.scale || enemy_random.load()!=p.random) return false;
    if(!p.random) return true;
    return enemy_random_scale_min.load()==p.scale_min && enemy_random_scale_max.load()==p.scale_max &&
        enemy_random_hp_min.load()==p.hp_min && enemy_random_hp_max.load()==p.hp_max && enemy_elite_chance.load()==p.elite;
}
// Volta todas as opcoes dos inimigos (todos, aleatorio e elite) ao padrao; o mod Editor por tipo tem botao proprio.
void ResetEnemyOptions() {
    enemy_hp.store(1);enemy_scale.store(1);enemy_random.store(false);
    enemy_random_scale_min.store(.8f);enemy_random_scale_max.store(1.3f);
    enemy_random_hp_min.store(1);enemy_random_hp_max.store(1.5f);
    enemy_elite_chance.store(0);enemy_elite_scale.store(2);enemy_elite_hp.store(3);
    LOGR("dev: enemy options reset to default");
}
void ResetEnemyButton() {
    if(ImGui::Button(Tr("Restaurar padrao dos inimigos"),MenuSize(280,30))) ResetEnemyOptions();
}
void ApplyPreset(const EnemyPreset& p) {
    enemy_hp.store(p.hp);enemy_scale.store(p.scale);enemy_random.store(p.random);
    enemy_random_scale_min.store(p.scale_min);enemy_random_scale_max.store(p.scale_max);
    enemy_random_hp_min.store(p.hp_min);enemy_random_hp_max.store(p.hp_max);enemy_elite_chance.store(p.elite);
    enemy_elite_scale.store(2);enemy_elite_hp.store(3);
    LOGR("dev: enemy preset %s",p.name);
}
void EnemiesPage() {
    Heading("Todos os inimigos","Vida e tamanho de todos os inimigos, com combinacoes prontas. Tudo so na memoria: o jogo original nao muda.");
    for(size_t i=0;i<std::size(kEnemyPresets);++i) {
        if(i) ImGui::SameLine();
        const bool current=PresetActive(kEnemyPresets[i]);
        if(current) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        if(ImGui::Button(Tr(kEnemyPresets[i].name),MenuSize(0,30))) ApplyPreset(kEnemyPresets[i]);
        if(current) ImGui::PopStyleColor();
    }
    ImGui::TextColored(muted_text,"%s",Tr("Variados e Caotico sorteiam cada inimigo; Elites deixa 15% deles com 2x tamanho e 3x vida."));
    Slider("Vida dos inimigos",enemy_hp,kMinEnemyHpFactor,kMaxEnemyHpFactor,"%.2fx");
    Slider("Tamanho dos inimigos",enemy_scale,kMinEnemyScale,kMaxEnemyScale,"%.2fx");
    ImGui::TextWrapped("%s",Tr("As camadas se multiplicam: todos x tipo (Por tipo) x sorteio (Aleatorio). Limites finais: tamanho 0,5x a 3x, vida 0,5x a 5x."));
    ImGui::TextColored(muted_text,"%s",Tr("Tamanho e so visual: a area de acerto continua a original. Vida vale para quem aparece depois: troque de sala."));
    ResetEnemyButton();
}
void DropsPage() {
    Heading("Drops e itens","Chance e quantidade do que os inimigos soltam ao morrer, e tamanho dos itens no chao. So na memoria.");
    Slider("Taxa de drop de itens",drop_rate,0,kMaxDropFactor,"%.2fx");
    ImGui::TextColored(muted_text,"%s",Tr("Multiplica a chance de cada item (a sorte do personagem continua contando). 0x desliga os itens."));
    Slider("Chance de cristais dos Innocent Devils",crystal_rate,0,kMaxDropFactor,"%.2fx");
    Slider("Cristais por drop",crystal_amount,1,kMaxCrystalAmount,"%.2fx");
    ImGui::TextColored(muted_text,"%s",Tr("Cristais de evolucao so caem com um Innocent Devil em campo. O jogo junta no maximo 4 cristais por drop."));
    Toggle("Itens aleatorios nos drops",random_drops);
    ImGui::TextColored(muted_text,"%s",Tr("Troca o item que o inimigo soltaria por outro da tabela de drops do proprio jogo."));
    Slider("Tamanho dos itens no chao",item_scale,kMinItemScale,kMaxItemScale,"%.2fx");
    if(ImGui::Button(Tr("Itens pela metade"),MenuSize(200,30))) item_scale.store(.5f);
    ImGui::SameLine();
    if(ImGui::Button(Tr("Tamanho original"),MenuSize(200,30))) item_scale.store(1);
    ImGui::TextColored(muted_text,"%s",Tr("So o visual muda: a area de pegar continua a original. Vale para itens que caem depois da mudanca."));
    ImGui::TextWrapped("%s",Tr("Valem para inimigos que morrem depois da mudanca. Salvar opcoes grava tudo."));
}
void EnemyRandomPage() {
    Heading("Aleatorio","Cada inimigo sorteia tamanho e vida ao aparecer e mantem o sorteio ate sumir.");
    Toggle("Sortear tamanho e vida de cada inimigo",enemy_random);
    ImGui::BeginDisabled(!enemy_random.load());
    ImGui::SeparatorText(Tr("Faixas do sorteio"));
    Slider("Tamanho minimo",enemy_random_scale_min,kMinEnemyScale,kMaxEnemyScale,"%.2fx");
    Slider("Tamanho maximo",enemy_random_scale_max,kMinEnemyScale,kMaxEnemyScale,"%.2fx");
    Slider("Vida minima",enemy_random_hp_min,kMinEnemyHpFactor,kMaxEnemyHpFactor,"%.2fx");
    Slider("Vida maxima",enemy_random_hp_max,kMinEnemyHpFactor,kMaxEnemyHpFactor,"%.2fx");
    ImGui::SeparatorText(Tr("Elites"));
    Slider("Chance de elite",enemy_elite_chance,0,100,"%.0f%%");
    Slider("Tamanho extra da elite",enemy_elite_scale,kMinEnemyScale,kMaxEnemyScale,"%.2fx");
    Slider("Vida extra da elite",enemy_elite_hp,kMinEnemyHpFactor,kMaxEnemyHpFactor,"%.2fx");
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s",Tr("Minimo maior que maximo e trocado sozinho. Elite multiplica por cima do sorteio. Vale para inimigos que aparecem depois da mudanca."));
    ResetEnemyButton();
    ImGui::SeparatorText(Tr("Trocar tipos de inimigo"));
    ImGui::TextColored(muted_text,"%s",Tr("Em pesquisa: trocar um inimigo por outro exige carregar modelos que a sala nao tem, com risco de travar. Vem depois do estudo das posicoes."));
}
// Liga ou desliga o mod Editor; cria a pasta na primeira vez.
void SetEditorMod(bool enabled) {
    auto find=[]{for(size_t i=0;i<Mods().size();++i) if(Mods()[i].name==kEditorMod) return int(i);return -1;};
    if(find()<0) { if(!enabled || !SaveEnemyEdits()) return; ReloadMods(); }
    if(const int index=find();index>=0) EnableMod(size_t(index),enabled);
}
void EnemyEditorPage() {
    Heading("Por tipo","Vida e tamanho por tipo de inimigo, como mod: so na memoria, gravado em mods/Editor/enemies.toml. Desligar o mod volta ao jogo original.");
    bool on=EnemyEditorActive();
    if(ImGui::Checkbox(Tr("Aplicar edicoes (mod Editor)"),&on)) SetEditorMod(on);
    ImGui::SameLine();
    if(ImGui::Button(Tr("Salvar edicoes"),MenuSize(160,0)) && SaveEnemyEdits()) {
        ReloadMods();NotifyToast("Edicoes dos inimigos salvas em mods/Editor/enemies.toml.");
    }
    ImGui::TextWrapped("%s",Tr("A lista mostra os inimigos que ja apareceram nesta sessao e os salvos no mod. Tamanho muda na hora; vida vale para quem nasce depois (troque de sala). So o visual cresce: a area de acerto e a colisao continuam as originais."));
    if(!on) ImGui::TextColored(crimson,"%s",Tr("Mod Editor desligado: os ajustes abaixo ficam guardados, mas o jogo usa os valores originais."));
    const auto rows=EnemyEditorRows();
    if(rows.empty()) ImGui::TextColored(muted_text,"%s",Tr("Nenhum inimigo visto ainda: entre numa sala com inimigos."));
    else if(ImGui::Button(Tr("Voltar todos os tipos ao original"),MenuSize(300,30)))
        for(const auto& row:rows) SetEnemyEdit(row.type,1,1);
    for(const auto& row:rows) {
        ImGui::PushID(int(row.type));
        char header[96];
        std::snprintf(header,sizeof(header),Tr("%s (tipo %u)%s"),row.edit.name.empty() ? Tr("Inimigo") : row.edit.name.c_str(),row.type,
            row.seen ? "" : Tr(" - salvo, nao visto nesta sessao"));
        ImGui::SeparatorText(header);
        float hp=row.edit.hp,scale=row.edit.scale;
        ImGui::TextUnformatted(Tr("Vida"));ImGui::SetNextItemWidth(-1);
        bool changed=ImGui::SliderFloat("##hp",&hp,kMinEnemyHpFactor,kMaxEnemyHpFactor,"%.2fx");
        ImGui::TextUnformatted(Tr("Tamanho"));ImGui::SetNextItemWidth(-1);
        changed|=ImGui::SliderFloat("##scale",&scale,kMinEnemyScale,kMaxEnemyScale,"%.2fx");
        if(changed) SetEnemyEdit(row.type,hp,scale);
        if(!row.edit.Neutral() && ImGui::Button(Tr("Voltar ao original"),MenuSize(200,0))) SetEnemyEdit(row.type,1,1);
        ImGui::PopID();
    }
}
void HudPage() {
    Heading("HUD e pets", "Tamanho e visibilidade da interface original.");
    Toggle("Mostrar HUD original do jogo",hud_enabled);
    Toggle("Manter vida/MP sempre visiveis",hud_always_visible);
    if(hud_always_visible)ImGui::TextWrapped("%s",Tr("Mantem as barras visiveis fora de combate. Desative para voltar ao temporizador original."));
    Slider("Escala do HUD e molduras",hud_scale,.35f,1.0f,"%.2fx");
    if(ImGui::Button(Tr("Usar HUD em 50%"))) hud_scale.store(.5f);
    ImGui::BeginDisabled(hud_always_visible.load());
    Toggle("Ocultar HUD mais rapido fora de combate",hud_quick_hide);
    if(hud_quick_hide) Slider("Espera para ocultar HUD",hud_hide_delay,.2f,5.0f,"%.1f s");
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s",Tr("Usa o temporizador original do jogo; dano e outros eventos continuam exibindo a interface."));
}
void CameraPage() {
    Heading("Camera", "Enquadramento, camera livre e orbital experimental.");
    int camera=camera_mode.load();
    const char* cameras[]{Tr("Original"),Tr("Cinematica (inspirada em Lords of Shadow 1)"),Tr("Proxima (inspirada em Lords of Shadow 2)"),Tr("Personalizada"),Tr("Livre"),Tr("Orbital em terceira pessoa")};
    if(ImGui::Combo(Tr("Camera"),&camera,cameras,6)) camera_mode.store(camera);
    if(camera==3 || camera==5) {
        Slider("Distancia da camera",camera_distance,.5f,2.5f,"%.2fx");
        Slider("Altura da camera",camera_height,-200,300,"%.0f");
    }
    if(camera>=3) Slider("Campo de visao relativo",camera_field,.65f,1.5f,"%.2fx");
    if(camera==3 || camera==4) {
        Slider("Angulo horizontal",camera_yaw,-cod_view::kMaxCameraYaw,cod_view::kMaxCameraYaw,Tr("%.0f graus"));
        Slider("Inclinacao",camera_pitch,-cod_view::kMaxCameraPitch,cod_view::kMaxCameraPitch,Tr("%.0f graus"));
        Slider("Rotacao do horizonte",camera_roll,-cod_view::kMaxCameraRoll,cod_view::kMaxCameraRoll,Tr("%.0f graus"));
        if(ImGui::Button(Tr("Zerar angulo, inclinacao e rotacao"))) { camera_yaw=0;camera_pitch=0;camera_roll=0; }
        ImGui::TextWrapped("%s",Tr(camera==3 ? "Angulo e inclinacao giram a camera em volta do personagem; rotacao inclina o horizonte. O controle continua no referencial original, entao angulos grandes deixam o movimento torto."
                                     : "Somados ao olhar da camera livre; rotacao inclina o horizonte."));
    }
    if(camera==5) {
        Slider("Acompanhamento suave",orbit_smoothing,1,20,"%.1f");
        ImGui::TextWrapped("%s",Tr("Mouse / analogico direito: olhar independente. Apos 1,5 s sem olhar manual, volta suavemente para tras do personagem. WASD / analogico esquerdo: movimento. Usa colisao nativa para encurtar a distancia junto a paredes."));
    }
    if(camera==4) {
        Slider("Velocidade da camera livre",camera_fly_speed,100,2000,"%.0f");
        ImGui::TextWrapped("%s",Tr("Feche o painel: WASD move, Q/E desce/sobe, mouse olha. Controle: analogicos movem/olham, LT/RT descem/sobem. O personagem fica sem comandos; o mundo continua rodando."));
    }
    if(camera) ImGui::TextWrapped("%s",Tr("Experimental: perfis de enquadramento, sem copiar o sistema dos outros jogos. Pode afetar cenas com camera ativa e atravessar paredes. Original restaura o comportamento nativo."));
}
void Diagnostics(double fps) {
    Heading("Diagnostico", "Informacoes da partida e captura de problemas.");
    ImGui::Text(Tr("Desempenho: %.0f FPS"),fps);
    if(kernel::g_base) {
        const auto stage=static_cast<int8_t>(kernel::g_base[0xB234F4]);
        const auto room=static_cast<int8_t>(kernel::g_base[0xB234F5]);
        const char* area=AreaName(stage);
        ImGui::TextColored(gold,Tr("%s    Mapa ID %d    Sala ID %d"),area ? area : Tr("Area sem nome"),int(stage),int(room));
        ImGui::TextColored(muted_text,"%s",Tr("Nomes vindos dos teleportes do jogo; mapas sem teleporte mostram so o ID."));
    }
    Toggle("Registrar desempenho no log (1 vez por segundo)",perf_log);
    ImGui::TextColored(muted_text,"%s",Tr("Ligue, jogue o trecho que trava por 1 minuto e salve um relato: o log vai junto. So vale nesta sessao."));
    Toggle("Pesquisar vida dos inimigos (log)",enemy_probe);
    ImGui::TextColored(muted_text,"%s",Tr("Ligue, bata em 2 ou 3 inimigos ate morrerem e salve um relato. O log mostra quais campos do alvo caem a cada golpe."));
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if (ImGui::Button(Tr("Capturar e relatar bug  [F8 / Print Screen]"), MenuSize(-1,36))) { CaptureBug(); ShowPage(1); }
    ImGui::TextColored(muted_text,"%s",Tr("O mundo continua rodando. Pause pelo jogo antes de abrir as ferramentas."));
}
// Captura salva (BMP) cabendo na largura disponivel e na altura maxima pedida.
void DrawPreview(const std::filesystem::path& bmp, float max_height) {
    RequestReportPreview(bmp);
    const auto preview=CurrentReportPreview();
    if(preview.path!=bmp) { ImGui::TextDisabled("%s",Tr("Carregando a previa...")); return; }
    if(!preview.texture || !preview.width || !preview.height) { ImGui::TextColored(crimson,"%s",Tr("Nao foi possivel abrir a captura.")); return; }
    const float aspect=float(preview.height)/float(preview.width);
    float width=ImGui::GetContentRegionAvail().x,height=width*aspect;
    if(height>max_height) { height=std::max(40.0f,max_height);width=height/aspect; }
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(preview.texture)),{width,height});
}
void OpenFolder(const std::filesystem::path& folder) {
    // Caminho completo do explorer: sem ele o Windows procura primeiro na pasta do exe.
    wchar_t windows[MAX_PATH]{};
    const UINT length=GetWindowsDirectoryW(windows,MAX_PATH);
    if(!length || length>=MAX_PATH) return;
    const std::wstring explorer=std::wstring(windows)+L"\\explorer.exe";
    std::wstring command=L"\""+explorer+L"\" \""+folder.wstring()+L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if(!CreateProcessW(explorer.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process)) return;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
}
std::string ReadText(const std::filesystem::path& path, size_t limit) {
    std::ifstream input(path,std::ios::binary);
    std::string text(limit,'\0');
    input.read(text.data(),std::streamsize(limit));
    text.resize(size_t(std::max<std::streamsize>(0,input.gcount())));
    return text;
}
void ReportsGallery() {
    Heading("Capturas salvas", "Relatos guardados em reports/, do mais novo para o mais antigo. Print Screen ou F8 criam um novo.");
    static std::vector<ReportEntry> reports;
    static double refreshed=-10;
    static std::string selected,selected_text;
    if(ImGui::GetTime()-refreshed>2) { reports=ListReports(Root()/"reports",12);refreshed=ImGui::GetTime(); }
    if(reports.empty()) { ImGui::TextDisabled("%s",Tr("Nenhuma captura ainda.")); return; }
    auto current=std::find_if(reports.begin(),reports.end(),[](const ReportEntry& entry) { return entry.folder==selected; });
    if(current==reports.end()) { current=reports.begin();selected=current->folder;selected_text.clear(); }
    const auto folder=Root()/"reports"/selected;
    if(selected_text.empty()) selected_text=ReadText(folder/"report.txt",16384);
    if(current->screenshot) DrawPreview(folder/"screenshot.bmp",ImGui::GetContentRegionAvail().y*.42f);
    else ImGui::TextDisabled("%s",Tr("Este relato nao tem imagem."));
    ImGui::TextColored(gold,"%s",current->title.c_str());
    ImGui::TextColored(muted_text,"reports/%s",selected.c_str());
    if(ImGui::Button(Tr("Abrir pasta"),MenuSize(140,30))) OpenFolder(folder);
    ImGui::SameLine();
    if(ImGui::Button(Tr("Copiar relato"),MenuSize(160,30))) ImGui::SetClipboardText(selected_text.c_str());
    if(ImGui::TreeNode(Tr("Texto do relato"))) { ImGui::TextWrapped("%s",selected_text.empty() ? Tr("(sem report.txt)") : selected_text.c_str());ImGui::TreePop(); }
    ImGui::Separator();
    for(const auto& entry:reports) {
        char label[256];
        std::snprintf(label,sizeof(label),"%s%s  |  %s##%s",entry.screenshot ? "[img] " : "",entry.folder.c_str(),entry.title.c_str(),entry.folder.c_str());
        // o direcional ja troca a previa ao passar pela lista; A ou clique tambem selecionam
        if((ImGui::Selectable(label,entry.folder==selected) || ImGui::IsItemFocused()) && entry.folder!=selected) { selected=entry.folder;selected_text.clear(); }
    }
}
void LogsPage() {
    Heading("Logs e travamentos", "Final do log desta sessao e os travamentos salvos em CrashDumps. Nada e enviado.");
    static std::string tail;
    static std::vector<CrashEntry> crashes;
    static std::unordered_map<std::string,std::string> crash_logs;
    static double refreshed=-10;
    const auto crash_dir=Root()/"CrashDumps";
    const bool refresh=ImGui::Button(Tr("Atualizar"),MenuSize(130,30));
    if(refresh || ImGui::GetTime()-refreshed>3) {
        logx::Flush();
        tail=LogTail(logx::Path(),200);
        crashes=ListCrashes(crash_dir,10);
        refreshed=ImGui::GetTime();
    }
    ImGui::SameLine();
    if(ImGui::Button(Tr("Copiar log"),MenuSize(140,30))) ImGui::SetClipboardText(tail.c_str());
    ImGui::SameLine();
    if(ImGui::Button(Tr("Abrir pasta do log"),MenuSize(190,30))) OpenFolder(std::filesystem::path(logx::Path()).parent_path());
    ImGui::TextColored(muted_text,Tr("%s (ultimas 200 linhas)"),logx::Path().c_str());
    const float scale=ImGui::GetIO().FontGlobalScale;
    ImGui::InputTextMultiline("##log",tail.data(),tail.size()+1,{-1,std::max(120.0f*scale,ImGui::GetContentRegionAvail().y*.45f)},ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();
    ImGui::TextColored(gold,"%s",Tr("Travamentos"));
    ImGui::SameLine();
    if(ImGui::SmallButton(Tr("Abrir CrashDumps"))) OpenFolder(crash_dir);
    if(crashes.empty()) ImGui::TextDisabled("%s",Tr("Nenhum travamento registrado."));
    for(const auto& crash:crashes) {
        char label[192];
        std::snprintf(label,sizeof(label),"%s  (%.1f MB)%s",crash.name.c_str(),double(crash.bytes)/1048576.0,crash.log ? "  + log" : "");
        if(!crash.log) { ImGui::BulletText("%s",label);continue; }
        if(!ImGui::TreeNode(crash.name.c_str(),"%s",label)) continue;
        auto& text=crash_logs[crash.name];
        if(text.empty()) text=LogTail(crash_dir/(crash.name+".log"),40);
        if(ImGui::SmallButton(Tr("Copiar este log"))) ImGui::SetClipboardText(text.c_str());
        ImGui::TextWrapped("%s",text.c_str());
        ImGui::TreePop();
    }
}
void Reports() {
    Heading("Relatar bug", "Capture o quadro do jogo (Print Screen ou F8), descreva o problema e salve tudo nesta maquina.");
    if (ImGui::Button(Tr("Nova captura"), MenuSize(160,30))) CaptureBug();
    ImGui::SameLine();
    ImGui::BeginDisabled(CurrentReport().empty() || !description[0]);
    if (ImGui::Button(Tr("Salvar relato local"), MenuSize(200,30))) SaveReport(title, description);
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", Tr(ReportStatus().c_str()));
    std::error_code ec;
    const auto shot=CurrentReport()/"screenshot.bmp";
    if (!CurrentReport().empty() && std::filesystem::is_regular_file(shot,ec)) DrawPreview(shot,ImGui::GetContentRegionAvail().y*.35f);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##title", Tr("Titulo curto do problema"), title, sizeof(title));
    ImGui::TextUnformatted(Tr("Descreva o problema e como reproduzir:"));
    const float scale=ImGui::GetIO().FontGlobalScale;
    ImGui::InputTextMultiline("##description", description, sizeof(description), {-1,std::max(90.0f*scale,ImGui::GetContentRegionAvail().y-70*scale)});
    if (!CurrentReport().empty()) ImGui::TextWrapped(Tr("Pasta: reports/%s"), CurrentReport().filename().string().c_str());
    ImGui::TextColored(muted_text,"%s",Tr("Nenhum arquivo e enviado automaticamente."));
}
bool DrawCharacterSelection() {
    static bool armed=false;
    if (!CharacterChoosing()) { armed=false;return false; }
    ShutdownInput();
    if (!ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown) && !ImGui::IsKeyDown(ImGuiKey_Enter) &&
        !ImGui::IsKeyDown(ImGuiKey_Space) && !ImGui::IsMouseDown(0)) armed=true;
    const auto& io=ImGui::GetIO();
    const float width=std::min(720.0f,io.DisplaySize.x-32);
    ImGui::SetNextWindowPos({io.DisplaySize.x*.5f,io.DisplaySize.y*.5f},ImGuiCond_Always,{.5f,.5f});
    ImGui::SetNextWindowSize({width,std::min(490.0f,io.DisplaySize.y-32)},ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4{.065f,.035f,.045f,.99f});
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4{.30f,.07f,.10f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4{.48f,.13f,.17f,1});
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4{.96f,.92f,.85f,1});
    ImGui::Begin((std::string(Tr("Novo jogo - personagem"))+"###new_game").c_str(),nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove);
    Heading("Prepare sua partida", "Escolha o personagem. O nome escolhido será mantido.");
    ImGui::TextUnformatted(Tr("Escolha seu personagem"));
    ImGui::Spacing();
    ImGui::BeginDisabled(!armed);
    auto choose=[&](int character){
        new_game_character.store(character);
        new_game_phase.store(character_preview_active.exchange(false) ? NewGamePhase::Idle : NewGamePhase::Starting);
        LOGR("character: selected %s",character==2 ? "Trevor" : "Hector");
        ShutdownInput();
    };
    if (ImGui::Button("Hector",{(width-50)*.5f,62})) choose(0);
    if (ImGui::IsWindowAppearing()) ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button("Trevor Belmont",{(width-50)*.5f,62})) choose(2);
    ImGui::EndDisabled();
    ImGui::Spacing();
    ImGui::TextWrapped("%s",Tr("Hector: campanha original e Innocent Devils. Trevor: chicote e subarmas do modo Trevor."));
    ImGui::TextColored(gold,"%s",Tr("Controle: direcional e A/cruz. Teclado: Tab e Enter. Mouse: clique."));
    ImGui::TextWrapped("%s",Tr("A selecao vale apenas para este Novo Jogo. Use um slot vazio para salvar o novo progresso."));
    ImGui::End();ImGui::PopStyleColor(4);
    return true;
}
void Graphics() {
    Heading("Graficos", "Resolucao interna, filtragem e efeitos de apresentacao.");
    Toggle("Mostrar FPS na tela",show_fps);
    if(show_fps.load()) {
        int corner=fps_corner.load();
        const char* corners[]{Tr("Superior esquerdo"),Tr("Superior direito"),Tr("Inferior esquerdo"),Tr("Inferior direito")};
        if(ImGui::Combo(Tr("Canto do FPS"),&corner,corners,4)) fps_corner.store(corner);
    }
    auto& settings = app::Get();
    int mode = int(settings.display_mode);
    const char* modes[]{Tr("Tela cheia"), Tr("Sem bordas"), Tr("Janela")};
    if (ImGui::Combo(Tr("Tela"), &mode, modes, 3)) { settings.display_mode = app::DisplayMode(mode); app::ApplyDisplay(); }
    ImGui::TextDisabled("%s",Tr("Alt+Enter: alternar janela / tela cheia."));
    const char* resolutions[]{Tr("Area de trabalho"), "960 x 540", "1280 x 720", "1920 x 1080", "2560 x 1440", "3840 x 2160"};
    constexpr std::array<app::Mode,6> sizes{{{0,0},{960,540},{1280,720},{1920,1080},{2560,1440},{3840,2160}}};
    int resolution = 0;
    for (int i=0;i<6;i++) if (settings.width==sizes[size_t(i)].width && settings.height==sizes[size_t(i)].height) resolution=i;
    if (ImGui::Combo(Tr("Resolucao"), &resolution, resolutions, 6)) {
        settings.width=sizes[size_t(resolution)].width; settings.height=sizes[size_t(resolution)].height; app::ApplyDisplay();
    }
    int percent=int(settings.render_percent);
    if (ImGui::SliderInt(Tr("Resolucao interna"), &percent, 50, 200, "%d%%")) settings.render_percent=uint32_t(percent);
    int aniso=0;
    constexpr uint32_t levels[]{0,2,4,8,16};
    for (int i=0;i<5;i++) if (settings.aniso==levels[i]) aniso=i;
    const char* filters[]{Tr("Como o jogo pede"), "2x", "4x", "8x", "16x"};
    if (ImGui::Combo(Tr("Filtro anisotropico"), &aniso, filters, 5)) settings.aniso=levels[aniso];
    int cap=int(settings.fps_cap);
    if (ImGui::SliderInt(Tr("Limite de FPS"), &cap, 30, 240)) settings.fps_cap=uint32_t(cap);
    ImGui::Checkbox(Tr("Sincronizacao vertical"), &settings.vsync);
    int selected=shader.load();
    const char* shaders[]{Tr("Original"), Tr("Escala de cinza"), Tr("CRT leve (linhas de varredura)")};
    if (ImGui::Combo(Tr("Shader de apresentacao"), &selected, shaders, 3)) shader.store(selected);
    Slider("Brilho",brightness,-.3f,.3f,"%+.2f");
    Slider("Contraste",contrast,.5f,1.5f,"%.2fx");
    Slider("Saturacao",saturation,0,2,"%.2fx");
    if(ImGui::Button(Tr("Restaurar brilho, contraste e saturacao"))) { brightness=0;contrast=1;saturation=1; }
    ImGui::Spacing();
    ImGui::TextWrapped("%s",Tr("Aumentar a resolucao interna e a filtragem melhora a imagem, mas nao cria detalhes novos nas texturas. Packs HD entram pela aba Mods."));
}
void Audio() {
    Heading("Audio", "Volume e canais da saida nativa XAudio2.");
    Toggle("Sem som  [F9]", muted);
    Slider("Volume geral", volume, 0, 1, "%.2f");
    Slider("Balanco esquerda / direita", balance, -1, 1, "%.2f");
    if (ImGui::Button(Tr("Centralizar balanco"))) balance.store(0);
    Toggle("Silenciar quando a janela perde o foco",mute_unfocused);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextUnformatted(Tr("Saida do port: estereo, 48 kHz, float32."));
    ImGui::TextWrapped("%s",Tr("O Windows e o XAudio2 encaminham os dois canais para a saida padrao. O port ainda nao oferece surround 5.1/7.1 nativo."));
    ImGui::TextWrapped("%s",Tr("Os efeitos DSP do Xbox sao parcialmente reproduzidos. O log do relato identifica o formato e os canais usados nesta execucao."));
}
std::string KeyName(int key) {
    if (key==VK_LBUTTON) return Tr("Mouse esquerdo");
    if (key==VK_RBUTTON) return Tr("Mouse direito");
    char text[96]{};
    LONG code=LONG(MapVirtualKeyA(UINT(key), MAPVK_VK_TO_VSC)<<16);
    if (GetKeyNameTextA(code,text,sizeof(text))) return text;
    return std::string(Tr("Tecla ")) + std::to_string(key);
}
void ControllerPage() {
    Heading("Controle", "Escolha qual botao fisico aciona cada botao do jogo (jogador 1).");
    if(ControllerConnected()) ImGui::TextColored(gold,"%s",Tr("Controle conectado."));
    else ImGui::TextColored(crimson,"%s",Tr("Nenhum controle conectado na porta 1."));
    Toggle("L3 + R3 abre e fecha o menu",controller_shortcut);
    Toggle("Inverter camera horizontal (analogico direito)",invert_stick_x);
    if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",Tr("A opcao de inverter camera do jogo so troca o eixo vertical. Esta troca o horizontal."));
    ImGui::TextWrapped("%s",Tr("Menu dev: direcional navega, A/cruz confirma, B/circulo volta, L1/R1 troca de aba. O menu dev, L3 + R3 e Start + Back + LT + RT usam sempre os botoes fisicos, para o controle nunca ficar travado."));
    auto map=ControllerMapping();
    const char* layouts[]{Tr("Xbox (padrao)"),"PS2",Tr("personalizado")};
    ImGui::TextColored(gold,Tr("Layout atual: %s"),layouts[int(DetectControllerLayout(map))]);
    if(ImGui::Button(Tr("Layout Xbox"),MenuSize(140,30))) { CancelControllerCapture();SetControllerMapping(IdentityControllerMap()); }
    if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",Tr("Padrao do port: LB/RB sao branco/preto e LT/RT os gatilhos. Use com os icones originais (PS2-Botoes desligado)."));
    ImGui::SameLine();
    if(ImGui::Button(Tr("Layout PS2"),MenuSize(140,30))) { CancelControllerCapture();SetControllerMapping(Ps2ControllerMap()); }
    if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",Tr("L1/R1 viram os gatilhos do jogo e L2/R2 viram preto/branco, como no PS2. Combina com o pack PS2-Botoes."));
    ImGui::SameLine();
    if(ImGui::Button(Tr("Trocar A e B"),MenuSize(160,30))) { AssignButton(map,0,map[1]);SetControllerMapping(map); }
    const int capturing=ControllerCaptureTarget();
    if(capturing>=0) {
        ImGui::TextColored(gold,"%s",Tr("Solte tudo e pressione o botao fisico desejado (6 s). Esc cancela."));
        if(ImGui::IsKeyPressed(ImGuiKey_Escape,false)) CancelControllerCapture();
    } else ImGui::TextWrapped("%s",Tr("Selecione um botao do jogo e aperte A (ou clique); depois pressione o botao fisico. Quem usava esse botao fica com o antigo."));
    ImGui::Spacing();
    const char* game[]{"A","B","X","Y",Tr("Preto"),Tr("Branco"),"LT","RT",Tr("Direcional cima"),Tr("Direcional baixo"),
        Tr("Direcional esquerda"),Tr("Direcional direita"),"Start","Back",Tr("L3 (analogico esquerdo)"),Tr("R3 (analogico direito)")};
    const char* physical[]{Tr("A / Cruz"),Tr("B / Circulo"),Tr("X / Quadrado"),Tr("Y / Triangulo"),"RB / R1","LB / L1","LT / L2","RT / R2",
        Tr("Direcional cima"),Tr("Direcional baixo"),Tr("Direcional esquerda"),Tr("Direcional direita"),"Start / Options","Back / Share","L3","R3"};
    if (ImGui::BeginTable("controller_map", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(Tr("Botao do jogo"));ImGui::TableSetupColumn(Tr("Botao fisico"));
        ImGui::TableHeadersRow();
        for(int target=0;target<kPadButtonCount;++target) {
            ImGui::PushID(target);
            ImGui::TableNextRow();ImGui::TableNextColumn();
            const bool changed=map[size_t(target)]!=target;
            if(changed) ImGui::TextColored(gold,"%s",game[target]); else ImGui::TextUnformatted(game[target]);
            ImGui::TableNextColumn();
            const char* label=capturing==target ? Tr("Pressione um botao...") : physical[map[size_t(target)]];
            if(ImGui::Button(label,{-1,0}) && capturing<0) BeginControllerCapture(target);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::TextColored(muted_text,"%s",Tr("Salvar opcoes grava o mapeamento; perfis salvos tambem o levam."));
}
void Controls() {
    Heading("Teclado e mouse", "Teclado e mouse no jogador 1, junto com o controle conectado.");
    Toggle("Ativar teclado", keyboard);
    Toggle("Mouse controla o analogico da camera", mouse_camera);
    Toggle("Inverter mouse horizontal",invert_mouse_x);
    Toggle("Inverter mouse vertical",invert_mouse_y);
    if(ImGui::Button(Tr("Restaurar controles padrao"))) ResetBindings();
    Slider("Sensibilidade", sensitivity, .001f, .05f, "%.3f");
    ImGui::TextWrapped("%s",Tr("Menu fechado: WASD move, mouse controla a camera, setas sao o direcional (navegam nos menus do jogo), Enter confirma e Esc abre ou volta o menu do jogo. Menu aberto: teclado e mouse operam a interface; analogicos continuam no jogo."));
    ImGui::TextWrapped("%s",Tr("Clique no botao de uma acao e pressione a nova tecla. F1, F8, F9 e Esc ficam reservadas."));
    ImGui::Spacing();
    if (ImGui::BeginTable("bindings", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(Tr("Botao Xbox"));ImGui::TableSetupColumn(Tr("Acao"));ImGui::TableSetupColumn(Tr("Teclado / mouse"));
        ImGui::TableHeadersRow();
        const char* xbox[]{"A","B","X","Y",Tr("Preto (RB)"),Tr("Branco (LB)"),"LT","RT","Start","Back",Tr("LS cima"),Tr("LS baixo"),Tr("LS esquerda"),Tr("LS direita"),Tr("D-pad cima"),Tr("D-pad baixo"),Tr("D-pad esquerda"),Tr("D-pad direita"),"A"};
        const auto bindings = Bindings();
        for (size_t i=0;i<bindings.size();i++) {
            const auto& binding=bindings[i]; ImGui::PushID(int(i));
            ImGui::TableNextRow(); ImGui::TableNextColumn();
            const ImVec4 colors[]{{.35f,.85f,.35f,1},{1,.3f,.3f,1},{.3f,.6f,1,1},{1,.85f,.25f,1}};
            const size_t face=bindings[i].button<4 ? size_t(bindings[i].button) : 4;
            ImGui::TextColored(face<4 ? colors[face] : gold,"%s",i<std::size(xbox) ? xbox[i] : "");
            ImGui::TableNextColumn();ImGui::TextUnformatted(Tr(binding.action));
            ImGui::TableNextColumn();
            if (ImGui::Button(KeyName(binding.key).c_str(), {-1,0})) BeginBinding(int(i));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
void Appearance() {
    Heading("Aparencia", "Arraste a barra de titulo para mover. Use o canto inferior direito para redimensionar.");
    int menu_language=menu_english.load() ? 1 : 0;
    const char* menu_languages[]{"Português (Brasil)","English"};
    if(ImGui::Combo(Tr("Idioma do menu"),&menu_language,menu_languages,2)) SetMenuEnglish(menu_language==1);
    ImGui::TextColored(muted_text,"%s",Tr("Muda so o texto destas ferramentas. Os botoes PT-BR e English no rodape fazem o mesmo."));
    Toggle("Pausar o jogo com o menu aberto",pause_with_menu);
    ImGui::TextColored(muted_text,"%s",Tr("Desligado (padrao): o jogo continua rodando; os analogicos andam e giram a camera e os botoes ficam com o menu."));
    ImGui::SliderFloat(Tr("Opacidade do painel"),&menu_preferences.opacity,.25f,1,"%.2f");
    ImGui::ColorEdit3(Tr("Fundo"),menu_preferences.background.data());
    ImGui::ColorEdit3(Tr("Cor de selecao"),menu_preferences.accent.data());
    ImGui::ColorEdit3(Tr("Destaques e foco"),menu_preferences.highlight.data());
    ImGui::TextWrapped("%s",Tr("Posicao, tamanho e cores ficam nas suas opcoes. Salvar opcoes grava imediatamente; fechar o painel tambem grava a geometria."));
    if(ImGui::Button(Tr("Restaurar aparencia padrao"))) {
        menu_preferences=MenuPreferences{};menu_geometry_reset=true;
    }
    ImGui::Spacing();
    ImGui::TextWrapped("%s",Tr("Controle: L1/R1 troca de aba. Direcional navega. A confirma e B volta. Movimento e tamanho da janela podem ser ajustados abaixo sem mouse."));
    ImGui::DragFloat(Tr("Posicao horizontal"),&menu_preferences.x,2,0,ImGui::GetIO().DisplaySize.x-160);
    const bool moved_x=ImGui::IsItemEdited();
    ImGui::DragFloat(Tr("Posicao vertical"),&menu_preferences.y,2,0,ImGui::GetIO().DisplaySize.y-80);
    const bool moved_y=ImGui::IsItemEdited();
    ImGui::DragFloat(Tr("Largura"),&menu_preferences.width,4,640,ImGui::GetIO().DisplaySize.x);
    const bool resized_x=ImGui::IsItemEdited();
    ImGui::DragFloat(Tr("Altura"),&menu_preferences.height,4,420,ImGui::GetIO().DisplaySize.y);
    if(moved_x || moved_y || resized_x || ImGui::IsItemEdited()) menu_geometry_reset=true;
}
void ProfilesPage() {
    Heading("Perfis", "Salve combinacoes de opcoes. Restaurar padrao desativa as alteracoes do port sem apagar seus perfis.");
    static char status[128] = {};
    static bool status_ok = true;
    auto names=UserProfileNames();
    std::array<std::string,5> labels{Tr("Padrao (vanilla)"),"Dev (Godmode)",Tr("Perfil 1"),Tr("Perfil 2"),Tr("Perfil 3")};
    for(size_t i=0;i<names.size();++i)if(!names[i].empty())labels[i+2]=names[i];
    std::array<const char*,5> items{};for(size_t i=0;i<items.size();++i)items[i]=labels[i].c_str();
    int selection=SelectedProfile();
    if(ImGui::Combo(Tr("Perfil"),&selection,items.data(),int(items.size()))) {
        SetSelectedProfile(selection);
        const std::string& name=selection>=2?names[size_t(selection-2)]:selection==1?labels[1]:labels[0];
        std::snprintf(profile_name,sizeof(profile_name),"%s",name.empty()?"Meu perfil":name.c_str());
        status[0]=0;status_ok=true;
    }
    if(ImGui::Button(Tr("Aplicar perfil"),MenuSize(190,36))) {
        const bool applied=ApplyProfile(SelectedProfile());
        std::snprintf(status,sizeof(status),"%s",Tr(applied?"Perfil aplicado.":"Este perfil ainda nao tem dados salvos."));
        status_ok=applied;
    }
    ImGui::SameLine();
    if(ImGui::Button(Tr("Ativar perfil Dev"),MenuSize(190,36))) {
        ApplyProfile(1);
        const bool godmode_active=godmode_ready.load();
        std::snprintf(status,sizeof(status),"%s",Tr(godmode_active?
            "Perfil Dev ativo: Godmode e modificacoes do port ligados.":
            "Perfil Dev ativo, mas Godmode nao passou na validacao desta versao."));
        status_ok=godmode_active;
    }
    ImGui::InputText(Tr("Nome do perfil salvo"),profile_name,sizeof(profile_name));
    ImGui::BeginDisabled(SelectedProfile()<2);
    if(ImGui::Button(Tr("Salvar opcoes atuais neste perfil"),MenuSize(285,36))) {
        if(SaveUserProfile(SelectedProfile()-2,profile_name)) {
            app::Save();std::snprintf(status,sizeof(status),"%s",Tr("Perfil salvo. As opcoes atuais foram guardadas."));
            status_ok=true;
        } else {std::snprintf(status,sizeof(status),"%s",Tr("Digite um nome antes de salvar o perfil."));status_ok=false;}
    }
    ImGui::EndDisabled();
    if(status[0])ImGui::TextColored(status_ok?gold:crimson,"%s",status);
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if(ImGui::Button(Tr("Restaurar padrao vanilla"),MenuSize(245,40))) {
        ApplyProfile(0);std::snprintf(status,sizeof(status),"%s",Tr("Padroes restaurados. Voce pode alterar as opcoes agora."));
        status_ok=true;
    }
    ImGui::TextWrapped("%s",Tr("O perfil vanilla restaura imagem, controles, audio, camera e opcoes do port aos valores originais. Seus perfis salvos permanecem disponiveis, e voce pode alterar qualquer opcao logo depois."));
    ImGui::TextWrapped("%s",Tr("O perfil Dev liga Godmode, PT-BR, minimapa rotativo e HUD em 50%. Mantem a camera original para evitar o modo orbital que voce relatou como bugado."));
}
void SavesPage() {
    Heading("Saves e sessao", "Arquivos de progresso do jogo. Carregamento usa o menu original.");
    ImGui::TextWrapped("%s",Tr("Para carregar outro slot, reinicie no titulo e escolha Load Game. O progresso que ainda nao foi salvo sera perdido."));
    if(ImGui::Button(Tr("Reiniciar no titulo / carregar save"))) ImGui::OpenPopup("###confirm_restart");
    if(ImGui::BeginPopupModal((std::string(Tr("Confirmar reinicio"))+"###confirm_restart").c_str(),nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(Tr("O progresso nao salvo sera perdido. Seus saves gravados ficam preservados."));
        if(ImGui::Button(Tr("Reiniciar no titulo"))) { app::Save();RequestTitleRestart();ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();if(ImGui::Button(Tr("Continuar jogando"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(Tr("Arquivos locais em hdd (somente leitura)"));
    static std::vector<std::string> files;
    static bool scanned=false;
    const bool refresh=ImGui::Button(Tr("Atualizar lista de arquivos"));
    if(!scanned || refresh) {
    scanned=true;files.clear();
    // Consulta apenas metadados; nunca abre, substitui ou apaga progresso.
    std::error_code ec;
    const auto root=Root()/"hdd";
    size_t count=0;
    for(std::filesystem::recursive_directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,ec),end;it!=end && !ec;it.increment(ec)) {
        if(it->is_symlink(ec)) { if(it->is_directory(ec)) it.disable_recursion_pending();continue; }
        if(!it->is_regular_file(ec)) continue;
        files.push_back(it->path().lexically_relative(root).string()+" ("+std::to_string(it->file_size(ec))+" bytes)");
        if(++count>=100){files.push_back(Tr("Lista limitada aos primeiros 100 arquivos."));break;}
    }
    }
    for(const auto& file:files) ImGui::TextWrapped("%s",file.c_str());
    if(files.empty()) ImGui::TextUnformatted(Tr("Nenhum arquivo de save encontrado."));
    ImGui::Spacing();
    ImGui::TextWrapped("%s",Tr("Savestate completo ainda exige suporte do motor. Esses arquivos sao saves normais, nao um estado instantaneo da partida."));
}
void ModsPage() {
    Heading("Mods", "Arquivos adicionais, sem sobrescrever os assets originais.");
    if (ImGui::Button(Tr("Recarregar lista de mods"))) ReloadMods();
    ImGui::TextWrapped("%s",Tr("Coloque arquivos em mods/NomeDoMod/files/ com o mesmo caminho do jogo. Ative abaixo e reinicie para recarregar assets ja em cache."));
    ImGui::Spacing();
    if (Mods().empty()) ImGui::TextColored(muted_text,"%s",Tr("Nenhum mod instalado. Consulte MODDING.md na pasta do projeto."));
    for (size_t i=0;i<Mods().size();i++) {
        if (Mods()[i].name==kTranslationTextureMod) {
            bool active=ptbr_enabled.load();
            ImGui::BeginDisabled();ImGui::Checkbox(Mods()[i].name.c_str(),&active);ImGui::EndDisabled();
            ImGui::SameLine();ImGui::TextColored(muted_text,"%s",Tr("segue Opcoes > Idioma > Português"));
            continue;
        }
        bool enabled=Mods()[i].enabled;
        if (ImGui::Checkbox(Mods()[i].name.c_str(), &enabled)) EnableMod(i,enabled);
    }
    ImGui::Spacing();
    ImGui::TextWrapped("%s",Tr("Se dois mods substituirem o mesmo arquivo, o nome em ordem alfabetica mais alta tem prioridade. So arquivos do disco do jogo sao substituidos; saves ficam fora dessa camada."));
    Toggle("Exportar texturas para criar um pack HD", texture_dump);
    ImGui::Text(Tr("Texturas exportadas: %u | Substituicoes HD carregadas: %u"),texture_exports.load(),texture_replacements.load());
    ImGui::TextWrapped("%s",Tr("Exporta ate 200 texturas por execucao para texture-dumps. Packs usam arquivos BMP nomeados pelo hash da textura; veja MODDING.md."));
}
void CheatsPage() {
    Heading("Cheats", "Patches de memoria para esta versao de Xbox, com valor esperado e reversao.");
    ImGui::TextWrapped("%s",Tr("Codigos GameShark de PS2 nao sao compativeis. Use cheats.toml com enderecos validados do Xbox USA. Cheats nao sao ativados automaticamente."));
    if (ImGui::Button(Tr("Recarregar cheats.toml"))) ReloadCheats();
    ImGui::Spacing();
    GameHacks();
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if (Cheats().empty()) ImGui::TextColored(muted_text,"%s",Tr("Nenhum perfil adicional em cheats.toml."));
    for (auto& cheat : Cheats()) {
        bool enabled=cheat.enabled;
        if (ImGui::Checkbox(cheat.name.c_str(), &enabled)) ToggleCheat(cheat,enabled);
        ImGui::SameLine(); ImGui::TextColored(muted_text,"%s",cheat.status.c_str());
    }
    ImGui::Spacing();
    ImGui::TextWrapped("%s",Tr("Ao desligar um cheat, o valor original so e restaurado se a memoria ainda contiver o valor escrito pelo patch. A seguranca depende de validar o endereco e o contexto."));
}
}

void ConfigureMenuFonts() {
    auto& io = ImGui::GetIO();
    char windows[MAX_PATH]{};
    const UINT length = GetWindowsDirectoryA(windows,MAX_PATH);
    if (!length || length >= MAX_PATH) { io.Fonts->AddFontDefault(); return; }
    const std::filesystem::path directory = std::filesystem::path(windows)/"Fonts";
    ImFontConfig config;
    config.OversampleH=3;config.OversampleV=2;
    // Mais pixels no atlas, mesmas metricas: ampliar nao estica glifos de 20px.
    config.RasterizerDensity=3.0f;
    if (std::filesystem::is_regular_file(directory/"segoeui.ttf")) {
        io.FontDefault=io.Fonts->AddFontFromFileTTF((directory/"segoeui.ttf").string().c_str(),20,&config);
    } else io.Fonts->AddFontDefault();
    if (std::filesystem::is_regular_file(directory/"segoeuib.ttf"))
        section_font=io.Fonts->AddFontFromFileTTF((directory/"segoeuib.ttf").string().c_str(),25,&config);
    if (std::filesystem::is_regular_file(directory/"georgia.ttf"))
        brand_font=io.Fonts->AddFontFromFileTTF((directory/"georgia.ttf").string().c_str(),34,&config);
}

void Draw(double fps) {
    static const bool checked_image=ValidateGodmodeImage();
    static const bool checked_character=ValidateCharacterSelection();
    (void)checked_image;
    (void)checked_character;
    RunIntegrationChecks();
    static bool preview=false;
    if (!preview && std::getenv("COD_CHARACTER_PREVIEW")) {
        preview=true;
        character_preview_active.store(true);
        new_game_phase.store(NewGamePhase::Choosing);
    }
    TickCheats();
    if(language_dirty.exchange(false))app::Save();
    DrawMinimap();
    DrawFps(fps);
    if (DrawCharacterSelection()) {DrawToast();return;}
    static bool was_open=false,editing_item=false;
    if (!menu_open) { if(was_open) app::Save();was_open=false;editing_item=false;DrawToast();return; }
    const bool was_editing=editing_item || ImGui::IsAnyItemActive();
    if(!was_open) { nav.level=MenuNav::Level::Titles;nav.focus=true; }
    was_open=true;
    if (!CurrentReport().empty() && ReportStatus().find("Capturando") == 0 && page!=1) ShowPage(1);
    const float scale=ImGui::GetIO().FontGlobalScale;
    ImGuiStyle& style=ImGui::GetStyle();
    menu_preferences.Sanitize();
    gold={menu_preferences.highlight[0],menu_preferences.highlight[1],menu_preferences.highlight[2],1};
    crimson={menu_preferences.accent[0],menu_preferences.accent[1],menu_preferences.accent[2],1};
    style.WindowRounding=2*scale;style.ChildRounding=0;style.FrameRounding=3*scale;
    style.WindowPadding={24*scale,22*scale};style.ItemSpacing={14*scale,12*scale};
    style.FramePadding={12*scale,7*scale};style.ScrollbarSize=12*scale;
    style.ItemInnerSpacing={6*scale,4*scale};style.IndentSpacing=21*scale;
    style.GrabMinSize=12*scale;style.ScrollbarRounding=3*scale;
    style.Colors[ImGuiCol_WindowBg]={.065f,.043f,.048f,.985f};
    style.Colors[ImGuiCol_ChildBg]={.065f,.043f,.048f,1};
    style.Colors[ImGuiCol_Border]={.38f,.29f,.18f,1};
    style.Colors[ImGuiCol_Separator]={.32f,.24f,.16f,1};
    style.Colors[ImGuiCol_Text]={.96f,.92f,.85f,1};
    style.Colors[ImGuiCol_TextDisabled]=muted_text;
    style.Colors[ImGuiCol_Button]={.25f,.09f,.12f,1};
    style.Colors[ImGuiCol_ButtonHovered]={.38f,.12f,.17f,1};
    style.Colors[ImGuiCol_ButtonActive]={.48f,.15f,.20f,1};
    style.Colors[ImGuiCol_FrameBg]={.13f,.085f,.095f,1};
    style.Colors[ImGuiCol_FrameBgHovered]={.20f,.12f,.13f,1};
    style.Colors[ImGuiCol_FrameBgActive]={.26f,.14f,.15f,1};
    style.Colors[ImGuiCol_Header]={.34f,.10f,.14f,1};
    style.Colors[ImGuiCol_HeaderHovered]={.42f,.13f,.17f,1};
    style.Colors[ImGuiCol_HeaderActive]={.46f,.15f,.19f,1};
    style.Colors[ImGuiCol_NavHighlight]=gold;
    style.Colors[ImGuiCol_ScrollbarBg]={.07f,.045f,.05f,1};
    style.Colors[ImGuiCol_ScrollbarGrab]={.36f,.27f,.19f,1};
    style.Colors[ImGuiCol_ScrollbarGrabHovered]={.54f,.40f,.25f,1};
    style.Colors[ImGuiCol_TextSelectedBg]={.40f,.14f,.18f,1};
    style.Colors[ImGuiCol_CheckMark]=gold; style.Colors[ImGuiCol_SliderGrab]=gold;
    style.Colors[ImGuiCol_WindowBg]={menu_preferences.background[0],menu_preferences.background[1],menu_preferences.background[2],menu_preferences.opacity};
    style.Colors[ImGuiCol_ChildBg]={0,0,0,0};
    style.Colors[ImGuiCol_Header]={crimson.x*.65f,crimson.y*.65f,crimson.z*.65f,.85f};
    style.Colors[ImGuiCol_HeaderHovered]=crimson;style.Colors[ImGuiCol_HeaderActive]=crimson;
    style.Colors[ImGuiCol_TitleBgActive]={crimson.x*.35f,crimson.y*.35f,crimson.z*.35f,menu_preferences.opacity};
    ImVec2 screen=ImGui::GetIO().DisplaySize;
    const auto condition=menu_geometry_reset ? ImGuiCond_Always : ImGuiCond_Once;
    ImGui::SetNextWindowSizeConstraints({std::min(640.0f,screen.x),std::min(420.0f,screen.y)},screen);
    ImGui::SetNextWindowSize({std::min(screen.x-24,menu_preferences.width),std::min(screen.y-24,menu_preferences.height)},condition);
    ImGui::SetNextWindowPos({std::clamp(menu_preferences.x,0.0f,std::max(0.0f,screen.x-std::min(screen.x-24,menu_preferences.width))),std::clamp(menu_preferences.y,0.0f,std::max(0.0f,screen.y-std::min(screen.y-24,menu_preferences.height)))},condition);
    menu_geometry_reset=false;
    bool open=true;
    if (ImGui::Begin((std::string(Tr("Castlevania | Ferramentas"))+"###dev_tools").c_str(), &open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
        const ImVec2 pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
        menu_preferences.x=pos.x;menu_preferences.y=pos.y;menu_preferences.width=size.x;menu_preferences.height=size.y;menu_preferences.positioned=true;
        ImGui::GetWindowDrawList()->AddRect({pos.x+6,pos.y+6},{pos.x+size.x-6,pos.y+size.y-6},IM_COL32(105,78,39,255));
        ImGui::PushFont(brand_font);ImGui::TextColored(gold,"Castlevania");ImGui::PopFont();
        ImGui::SameLine();ImGui::PushFont(section_font);ImGui::TextColored(crimson,"Curse of Darkness");ImGui::PopFont();
        char status[64];std::snprintf(status,sizeof(status),"%.0f FPS  |  %.2fx",fps,speed.load());
        ImGui::SameLine(std::max(ImGui::GetCursorPosX(),size.x-ImGui::CalcTextSize(status).x-24*scale));
        ImGui::TextColored(muted_text,"%s",status);
        if(kernel::g_base) {
            const int stage=static_cast<int8_t>(kernel::g_base[0xB234F4]),room=static_cast<int8_t>(kernel::g_base[0xB234F5]);
            if(const char* area=AreaName(stage)) ImGui::TextColored(muted_text,Tr("%s  /  Mapa %d  /  Sala %d"),area,stage,room);
            else ImGui::TextColored(muted_text,Tr("Mapa %d  /  Sala %d"),stage,room);
        }
        ImGui::Separator();
        const float footer=80*scale;
        constexpr int tab_count=int(std::size(kTabs));
        const bool popup=ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId);
        const int previous_page=page;
        const int step=int(ImGui::IsKeyPressed(ImGuiKey_GamepadR1,false))-int(ImGui::IsKeyPressed(ImGuiKey_GamepadL1,false));
        const bool back=ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight,false) || ImGui::IsKeyPressed(ImGuiKey_Escape,false);
        // Y (triangulo) abre e fecha o teclado na tela da busca; B fecha o teclado antes de voltar no menu.
        if(!was_editing && !popup && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp,false)) { search_keys=!search_keys;search_keys_focus=search_keys; }
        const bool close_keys=search_keys && back;
        if(close_keys) search_keys=false;
        StepMenuNav(nav,{.tab_step=step,.back=back && !close_keys,.busy=was_editing || popup || ControllerCaptureTarget()>=0},tab_count);
        // Abas no topo: L1/R1 ou clique; o direcional fica nos titulos e opcoes.
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav,true);
        for(int i=0;i<tab_count;++i) {
            if(i) ImGui::SameLine(0,6*scale);
            const ImVec2 size{ImGui::CalcTextSize(Tr(kTabs[i].label)).x+20*scale,28*scale};
            if(ImGui::Selectable(Tr(kTabs[i].label),nav.tab==i,ImGuiSelectableFlags_None,size)) {
                nav.tab=i;nav.title=0;nav.level=MenuNav::Level::Titles;nav.focus=true;
            }
        }
        ImGui::PopItemFlag();
        // Filtro por texto: lupa (clique ou Y) abre o teclado na tela; teclado fisico digita direto (Ctrl+F foca).
        const float lupa=28*scale;
        if(ImGui::Button("##lupa",{lupa,lupa})) { search_keys=!search_keys;search_keys_focus=search_keys; }
        {
            const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
            const ImVec2 center{a.x+(b.x-a.x)*.42f,a.y+(b.y-a.y)*.42f};
            const float radius=(b.x-a.x)*.22f;
            auto* draw=ImGui::GetWindowDrawList();
            draw->AddCircle(center,radius,IM_COL32(230,200,140,255),16,2*scale);
            draw->AddLine({center.x+radius*.7f,center.y+radius*.7f},{b.x-(b.x-a.x)*.2f,b.y-(b.y-a.y)*.2f},IM_COL32(230,200,140,255),2.5f*scale);
        }
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",Tr("Buscar opcao (Y no controle)"));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1);
        if(ImGui::IsKeyChordPressed(ImGuiMod_Ctrl|ImGuiKey_F)) ImGui::SetKeyboardFocusHere();
        bool search_enter=ImGui::InputTextWithHint("##filtro",Tr("Buscar opcao (Ctrl+F ou Y): vida, camera, minimapa..."),
            search_text,sizeof(search_text),ImGuiInputTextFlags_EnterReturnsTrue);
        if(search_keys) {
            // Teclado na tela: navegavel pelo direcional, A digita.
            constexpr const char* rows[]{"ABCDEFGHIJKLM","NOPQRSTUVWXYZ"};
            const ImVec2 key{30*scale,28*scale};
            for(const char* row:rows) {
                for(const char* c=row;*c;++c) {
                    if(c!=row) ImGui::SameLine(0,3*scale);
                    const char label[2]{*c,0};
                    if(search_keys_focus) { ImGui::SetKeyboardFocusHere();search_keys_focus=false; }
                    if(ImGui::Button(label,key)) SearchKeyPress(search_text,sizeof(search_text),char(*c+32));
                }
            }
            if(ImGui::Button(Tr("Espaco"),{key.x*3,key.y})) SearchKeyPress(search_text,sizeof(search_text),' ');
            ImGui::SameLine(0,3*scale);
            if(ImGui::Button(Tr("Apagar"),{key.x*3,key.y})) SearchKeyPress(search_text,sizeof(search_text),'\b');
            ImGui::SameLine(0,3*scale);
            if(ImGui::Button(Tr("Limpar"),{key.x*3,key.y})) SearchKeyPress(search_text,sizeof(search_text),0);
            ImGui::SameLine(0,3*scale);
            if(ImGui::Button(Tr("Abrir 1o"),{key.x*3,key.y})) search_enter=true;
            ImGui::SameLine(0,3*scale);
            if(ImGui::Button(Tr("Fechar"),{key.x*3,key.y})) search_keys=false;
        }
        if(search_text[0]) {
            ImGui::BeginChild("search",{0,-footer},ImGuiChildFlags_Border);
            int found=0,target=-1;
            const char* target_label=nullptr;
            for(int i=0;i<int(std::size(kMenuSearchIndex));++i) {
                const auto& entry=kMenuSearchIndex[i];
                const std::string path=PagePath(entry.page),row=std::string(Tr(entry.label))+"   |   "+path;
                // acha pelo nome nos dois idiomas e mostra no idioma do menu
                if(path.empty() || (!MenuSearchMatch(row,search_text) && !MenuSearchMatch(entry.label,search_text))) continue;
                ImGui::PushID(i);
                const bool pick=ImGui::Selectable(row.c_str()) || (search_enter && !found);
                ImGui::PopID();
                if(pick && target<0) { target=entry.page;target_label=entry.label; }
                ++found;
            }
            if(!found) ImGui::TextColored(muted_text,"%s",Tr("Nada encontrado. Tente outra palavra, sem acento ou so o comeco."));
            ImGui::EndChild();
            if(target>=0) {
                ShowPage(target);jump_label=target_label;search_text[0]=0;search_keys=false;
                LOGR("dev: menu search -> page %d '%s'",target,target_label);
            }
        } else {
        const auto& tab=kTabs[nav.tab];
        nav.title=std::clamp(nav.title,0,tab.count-1);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2{8*scale,4*scale});
        ImGui::BeginChild("categories",{180*scale,-footer},ImGuiChildFlags_Border);
        ImGui::TextDisabled("%s",Tr(tab.label));ImGui::Separator();
        for(int i=0;i<tab.count;++i) MenuTitle(nav,i,Tr(tab.titles[i].label),{0,29*scale});
        ImGui::EndChild();ImGui::PopStyleVar();ImGui::SameLine();
        page=tab.titles[nav.title].page;
        if(page!=15 && ControllerCaptureTarget()>=0) CancelControllerCapture();
        ImGui::BeginChild("content", {0,-footer},ImGuiChildFlags_Border);
        MenuOptionsFocus(nav);
        if(page!=previous_page)ImGui::SetScrollY(0);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x*.55f);
        switch(page) {case 0:General();break;case 1:Reports();break;case 2:Graphics();break;
            case 3:Audio();break;case 4:Controls();break;case 5:ModsPage();break;case 6:CheatsPage();break;
            case 7:Appearance();break;case 8:SavesPage();break;case 9:ProfilesPage();break;
            case 10:HudPage();break;case 11:CameraPage();break;case 12:Diagnostics(fps);break;
            case 13:ReportsGallery();break;case 14:LogsPage();break;case 15:ControllerPage();break;
            case 16:DifficultyPage();break;case 17:EnemyEditorPage();break;
            case 18:EnemiesPage();break;case 19:EnemyRandomPage();break;case 20:DropsPage();break;}
        ImGui::PopItemWidth();ImGui::EndChild();
        }
        ImGui::Separator();
        if(ImGui::Button(Tr("Salvar opcoes"),{160*scale,32*scale})) app::Save();
        ImGui::SameLine();
        if(ImGui::Button(Tr("Voltar ao jogo"),{170*scale,32*scale})) open=false;
        // Idioma do menu dev (so a interface das ferramentas; o idioma do jogo fica em Jogabilidade).
        for(const bool english:{false,true}) {
            ImGui::SameLine();
            const bool current=menu_english.load()==english;
            if(current) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
            if(ImGui::Button(english ? "English" : "PT-BR",{110*scale,32*scale}) && !current) SetMenuEnglish(english);
            if(current) ImGui::PopStyleColor();
        }
        ImGui::TextDisabled("%s",Tr("L1/R1: aba | Direcional: navegar | A: entrar/ajustar | B: voltar | L3+R3 ou F1: fechar"));
    }
    ImGui::End();
    editing_item=ImGui::IsAnyItemActive();
    if(!open) { menu_open.store(false); ShutdownInput(); app::Save();was_open=false; }
    DrawToast();
}
}
