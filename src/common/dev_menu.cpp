#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "app/app.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "dev_runtime.h"
#include "menu_input.h"

namespace dev {
namespace {
int page = 0;
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
    const std::string message=ToastMessage();
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

void Heading(const char* name, const char* subtitle) {
    ImGui::PushFont(section_font);
    ImGui::TextColored(gold, "%s", name);
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text,muted_text);
    ImGui::TextWrapped("%s", subtitle);
    ImGui::PopStyleColor();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
}
bool Toggle(const char* label, std::atomic<bool>& setting) {
    bool value = setting.load();
    if (!ImGui::Checkbox(label, &value)) return false;
    setting.store(value); return true;
}
bool Slider(const char* label, std::atomic<float>& setting, float minimum, float maximum, const char* format) {
    float value = setting.load();
    if (!ImGui::SliderFloat(label, &value, minimum, maximum, format)) return false;
    setting.store(value); return true;
}
void GameHacks() {
    ImGui::BeginDisabled(!godmode_ready);
    if (Toggle("Godmode: proteger a vida do jogador",godmode))
        LOGR("dev: godmode %s",godmode.load() ? "enabled" : "disabled");
    ImGui::EndDisabled();
    if (!godmode_ready) ImGui::TextColored(crimson,"A assinatura desta versao do jogo ainda nao foi confirmada.");
    else ImGui::TextColored(muted_text,"Danos bloqueados nesta sessao: %u",prevented_damage.load());
    ImGui::TextWrapped("Preserva cura e outros personagens. Impactos e animacoes continuam; nao reverte morte ja iniciada.");
    ImGui::Spacing();
    float factor = speed.load();
    if(ImGui::SliderFloat("Speedhack",&factor,.5f,2.0f,"%.2fx")) SetSpeed(factor);
    const float factors[]{.5f,1,1.5f,2};
    const char* labels[]{"0.5x","Normal","1.5x","2x"};
    for(int i=0;i<4;i++) {
        if(i) ImGui::SameLine();
        if(ImGui::Button(labels[i],MenuSize(82,30))) SetSpeed(factors[i]);
    }
    ImGui::TextWrapped("Escala os relogios do Xbox, timers, quadros e audio. O tom do som acompanha a velocidade. Vsync volta a valer em 1x.");
}
void General(double fps) {
    Heading("Opcoes de jogo", "Minimapa, interface e ferramentas da partida.");
    Toggle("Mostrar minimapa",minimap_enabled);
    Toggle("Girar minimapa conforme a camera",minimap_rotating);
    ImGui::TextDisabled("Menus originais: escala 100%%. Reducao inteligente adiada.");
    int language=TextLanguage();
    const char* languages[]{"Japanese","English","Portugues (Brasil) - parcial"};
    if(ImGui::Combo("Idioma dos textos",&language,languages,3))SelectTextLanguage(language);
    if(ptbr_enabled)ImGui::TextWrapped("Traducao inicial de opcoes, mensagens e itens. Acentos e titulos em textura ainda pendentes. Vozes originais preservadas.");
    Slider("Tamanho do minimapa",minimap_scale,.6f,1.6f,"%.2fx");
    Slider("Opacidade do minimapa",minimap_opacity,.2f,1.0f,"%.2f");
    ImGui::Separator();
    Toggle("Mostrar HUD original do jogo",hud_enabled);
    Slider("Escala do HUD",hud_scale,.35f,1.0f,"%.2fx");
    ImGui::SameLine();if(ImGui::Button("50%")) hud_scale.store(.5f);
    Toggle("Ocultar HUD mais rapido fora de combate",hud_quick_hide);
    if(hud_quick_hide) Slider("Espera para ocultar HUD",hud_hide_delay,.2f,5.0f,"%.1f s");
    ImGui::TextWrapped("Usa o temporizador original do jogo; dano e outros eventos continuam exibindo a interface.");
    ImGui::Separator();
    int camera=camera_mode.load();
    const char* cameras[]{"Original","Cinematica (inspirada em Lords of Shadow 1)","Proxima (inspirada em Lords of Shadow 2)","Personalizada","Livre","Orbital em terceira pessoa"};
    if(ImGui::Combo("Camera",&camera,cameras,6)) camera_mode.store(camera);
    if(camera==3 || camera==5) {
        Slider("Distancia da camera",camera_distance,.5f,2.5f,"%.2fx");
        Slider("Altura da camera",camera_height,-200,300,"%.0f");
    }
    if(camera>=3) Slider("Campo de visao relativo",camera_field,.65f,1.5f,"%.2fx");
    if(camera==5) {
        Slider("Acompanhamento suave",orbit_smoothing,1,20,"%.1f");
        ImGui::TextWrapped("Mouse / analogico direito: olhar independente. Apos 1,5 s sem olhar manual, volta suavemente para tras do personagem. WASD / analogico esquerdo: movimento. Usa colisao nativa para encurtar a distancia junto a paredes.");
    }
    if(camera==4) {
        Slider("Velocidade da camera livre",camera_fly_speed,100,2000,"%.0f");
        ImGui::TextWrapped("Feche o painel: WASD move, Q/E desce/sobe, mouse olha. Controle: analogicos movem/olham, LT/RT descem/sobem. O personagem fica sem comandos; o mundo continua rodando.");
    }
    if(camera) ImGui::TextWrapped("Experimental: perfis de enquadramento, sem copiar o sistema dos outros jogos. Pode afetar cenas com camera ativa e atravessar paredes. Original restaura o comportamento nativo.");
    ImGui::Separator();
    (void)fps;
    if(kernel::g_base) {
        const auto stage=static_cast<int8_t>(kernel::g_base[0xB234F4]);
        const auto room=static_cast<int8_t>(kernel::g_base[0xB234F5]);
        ImGui::TextColored(gold,"Mapa ID %d    Sala ID %d",int(stage),int(room));
        ImGui::TextColored(muted_text,"IDs internos do COD; nomes das areas ainda em identificacao.");
    }
    GameHacks();
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if (ImGui::Button("Capturar e relatar bug  [F8]", MenuSize(-1,36))) { CaptureBug(); page = 1; }
    ImGui::TextColored(muted_text,"O mundo continua rodando. Pause pelo jogo antes de abrir as ferramentas.");
}
void Reports() {
    Heading("Relatar bug", "Capture o quadro do jogo, descreva o problema e salve tudo nesta maquina.");
    if (ImGui::Button("Nova captura", MenuSize(160,30))) CaptureBug();
    ImGui::SameLine();
    ImGui::BeginDisabled(CurrentReport().empty() || !description[0]);
    if (ImGui::Button("Salvar relato local", MenuSize(200,30))) SaveReport(title, description);
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", ReportStatus().c_str());
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##title", "Titulo curto do problema", title, sizeof(title));
    ImGui::TextUnformatted("Descreva o problema e como reproduzir:");
    const float scale=ImGui::GetIO().FontGlobalScale;
    ImGui::InputTextMultiline("##description", description, sizeof(description), {-1,std::max(90.0f*scale,ImGui::GetContentRegionAvail().y-70*scale)});
    if (!CurrentReport().empty()) ImGui::TextWrapped("Pasta: reports/%s", CurrentReport().filename().string().c_str());
    ImGui::TextColored(muted_text, "Nenhum arquivo e enviado automaticamente.");
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
    ImGui::SetNextWindowSize({width,330},ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4{.065f,.035f,.045f,.99f});
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4{.30f,.07f,.10f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4{.48f,.13f,.17f,1});
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4{.96f,.92f,.85f,1});
    ImGui::Begin("Novo jogo - personagem",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove);
    Heading("Escolha seu personagem", "O nome escolhido sera mantido. A campanha comeca com o modo original do personagem.");
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
    ImGui::TextWrapped("Hector: campanha original e Innocent Devils. Trevor: chicote e subarmas do modo Trevor.");
    ImGui::TextColored(gold,"Controle: direcional e A/cruz. Teclado: Tab e Enter. Mouse: clique.");
    ImGui::TextWrapped("A selecao vale apenas para este Novo Jogo. Use um slot vazio para salvar o novo progresso.");
    ImGui::End();ImGui::PopStyleColor(4);
    return true;
}
void Graphics() {
    Heading("Graficos", "Resolucao interna, filtragem e efeitos de apresentacao.");
    auto& settings = app::Get();
    int mode = int(settings.display_mode);
    const char* modes[]{"Tela cheia", "Sem bordas", "Janela"};
    if (ImGui::Combo("Tela", &mode, modes, 3)) { settings.display_mode = app::DisplayMode(mode); app::ApplyDisplay(); }
    const char* resolutions[]{"Area de trabalho", "960 x 540", "1280 x 720", "1920 x 1080", "2560 x 1440", "3840 x 2160"};
    constexpr std::array<app::Mode,6> sizes{{{0,0},{960,540},{1280,720},{1920,1080},{2560,1440},{3840,2160}}};
    int resolution = 0;
    for (int i=0;i<6;i++) if (settings.width==sizes[size_t(i)].width && settings.height==sizes[size_t(i)].height) resolution=i;
    if (ImGui::Combo("Resolucao", &resolution, resolutions, 6)) {
        settings.width=sizes[size_t(resolution)].width; settings.height=sizes[size_t(resolution)].height; app::ApplyDisplay();
    }
    int percent=int(settings.render_percent);
    if (ImGui::SliderInt("Resolucao interna", &percent, 50, 200, "%d%%")) settings.render_percent=uint32_t(percent);
    int aniso=0;
    constexpr uint32_t levels[]{0,2,4,8,16};
    for (int i=0;i<5;i++) if (settings.aniso==levels[i]) aniso=i;
    const char* filters[]{"Como o jogo pede", "2x", "4x", "8x", "16x"};
    if (ImGui::Combo("Filtro anisotropico", &aniso, filters, 5)) settings.aniso=levels[aniso];
    int cap=int(settings.fps_cap);
    if (ImGui::SliderInt("Limite de FPS", &cap, 30, 240)) settings.fps_cap=uint32_t(cap);
    ImGui::Checkbox("Sincronizacao vertical", &settings.vsync);
    int selected=shader.load();
    const char* shaders[]{"Original", "Escala de cinza", "CRT leve (linhas de varredura)"};
    if (ImGui::Combo("Shader de apresentacao", &selected, shaders, 3)) shader.store(selected);
    ImGui::Spacing();
    ImGui::TextWrapped("Aumentar a resolucao interna e a filtragem melhora a imagem, mas nao cria detalhes novos nas texturas. Packs HD entram pela aba Mods.");
}
void Audio() {
    Heading("Audio", "Volume e canais da saida nativa XAudio2.");
    Toggle("Sem som  [F9]", muted);
    Slider("Volume geral", volume, 0, 1, "%.2f");
    Slider("Balanco esquerda / direita", balance, -1, 1, "%.2f");
    if (ImGui::Button("Centralizar balanco")) balance.store(0);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextUnformatted("Saida do port: estereo, 48 kHz, float32.");
    ImGui::TextWrapped("O Windows e o XAudio2 encaminham os dois canais para a saida padrao. O port ainda nao oferece surround 5.1/7.1 nativo.");
    ImGui::TextWrapped("Os efeitos DSP do Xbox sao parcialmente reproduzidos. O log do relato identifica o formato e os canais usados nesta execucao.");
}
std::string KeyName(int key) {
    if (key==VK_LBUTTON) return "Mouse esquerdo";
    if (key==VK_RBUTTON) return "Mouse direito";
    char text[96]{};
    LONG code=LONG(MapVirtualKeyA(UINT(key), MAPVK_VK_TO_VSC)<<16);
    if (GetKeyNameTextA(code,text,sizeof(text))) return text;
    return "Tecla " + std::to_string(key);
}
void Controls() {
    Heading("Controles", "Teclado e mouse no jogador 1, junto com o controle conectado.");
    Toggle("L3 + R3 abre e fecha o menu",controller_shortcut);
    ImGui::TextWrapped("Menu: direcional navega, A/cruz confirma, B/circulo volta. L1/R1 troca de secao. Analogicos continuam no jogo.");
    Toggle("Ativar teclado", keyboard);
    Toggle("Mouse controla o analogico da camera", mouse_camera);
    Toggle("Inverter mouse horizontal",invert_mouse_x);
    Toggle("Inverter mouse vertical",invert_mouse_y);
    if(ImGui::Button("Restaurar controles padrao")) ResetBindings();
    Slider("Sensibilidade", sensitivity, .001f, .05f, "%.3f");
    ImGui::TextWrapped("Menu fechado: WASD move e setas controlam a camera. Menu aberto: teclado e mouse operam a interface; analogicos continuam no jogo.");
    ImGui::TextWrapped("Clique no botao de uma acao e pressione a nova tecla. F1, F8, F9 e Esc ficam reservadas.");
    ImGui::Spacing();
    if (ImGui::BeginTable("bindings", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Botao Xbox");ImGui::TableSetupColumn("Acao");ImGui::TableSetupColumn("Teclado / mouse");
        ImGui::TableHeadersRow();
        const char* xbox[]{"A","B","X","Y","Preto (RB)","Branco (LB)","LT","RT","Start","Back", "LS cima","LS baixo","LS esquerda","LS direita","RS cima","RS baixo","RS esquerda","RS direita"};
        const auto bindings = Bindings();
        for (size_t i=0;i<bindings.size();i++) {
            const auto& binding=bindings[i]; ImGui::PushID(int(i));
            ImGui::TableNextRow(); ImGui::TableNextColumn();
            const ImVec4 colors[]{{.35f,.85f,.35f,1},{1,.3f,.3f,1},{.3f,.6f,1,1},{1,.85f,.25f,1}};
            ImGui::TextColored(i<4 ? colors[i] : gold,"%s",xbox[i]);
            ImGui::TableNextColumn();ImGui::TextUnformatted(binding.action);
            ImGui::TableNextColumn();
            if (ImGui::Button(KeyName(binding.key).c_str(), {-1,0})) BeginBinding(int(i));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
void Appearance() {
    Heading("Aparencia", "Arraste a barra de titulo para mover. Use o canto inferior direito para redimensionar.");
    ImGui::SliderFloat("Opacidade do painel",&menu_preferences.opacity,.25f,1,"%.2f");
    ImGui::ColorEdit3("Fundo",menu_preferences.background.data());
    ImGui::ColorEdit3("Cor de selecao",menu_preferences.accent.data());
    ImGui::ColorEdit3("Destaques e foco",menu_preferences.highlight.data());
    ImGui::TextWrapped("Posicao, tamanho e cores ficam nas suas opcoes. Salvar opcoes grava imediatamente; fechar o painel tambem grava a geometria.");
    if(ImGui::Button("Restaurar aparencia padrao")) {
        menu_preferences=MenuPreferences{};menu_geometry_reset=true;
    }
    ImGui::Spacing();
    ImGui::TextWrapped("Controle: L1/R1 troca de aba. Direcional navega. A confirma e B volta. Movimento e tamanho da janela podem ser ajustados abaixo sem mouse.");
    ImGui::DragFloat("Posicao horizontal",&menu_preferences.x,2,0,ImGui::GetIO().DisplaySize.x-160);
    const bool moved_x=ImGui::IsItemEdited();
    ImGui::DragFloat("Posicao vertical",&menu_preferences.y,2,0,ImGui::GetIO().DisplaySize.y-80);
    const bool moved_y=ImGui::IsItemEdited();
    ImGui::DragFloat("Largura",&menu_preferences.width,4,640,ImGui::GetIO().DisplaySize.x);
    const bool resized_x=ImGui::IsItemEdited();
    ImGui::DragFloat("Altura",&menu_preferences.height,4,420,ImGui::GetIO().DisplaySize.y);
    if(moved_x || moved_y || resized_x || ImGui::IsItemEdited()) menu_geometry_reset=true;
}
void ProfilesPage() {
    Heading("Perfis", "Salve combinacoes de opcoes. Restaurar padrao desativa as alteracoes do port sem apagar seus perfis.");
    static char status[128] = {};
    static bool status_ok = true;
    auto names=UserProfileNames();
    std::array<std::string,5> labels{"Padrao (vanilla)","Dev (Godmode)","Perfil 1","Perfil 2","Perfil 3"};
    for(size_t i=0;i<names.size();++i)if(!names[i].empty())labels[i+2]=names[i];
    std::array<const char*,5> items{};for(size_t i=0;i<items.size();++i)items[i]=labels[i].c_str();
    int selection=SelectedProfile();
    if(ImGui::Combo("Perfil",&selection,items.data(),int(items.size()))) {
        SetSelectedProfile(selection);
        const std::string& name=selection>=2?names[size_t(selection-2)]:selection==1?labels[1]:labels[0];
        std::snprintf(profile_name,sizeof(profile_name),"%s",name.empty()?"Meu perfil":name.c_str());
        status[0]=0;status_ok=true;
    }
    if(ImGui::Button("Aplicar perfil",MenuSize(190,36))) {
        const bool applied=ApplyProfile(SelectedProfile());
        std::snprintf(status,sizeof(status),"%s",applied?"Perfil aplicado.":"Este perfil ainda nao tem dados salvos.");
        status_ok=applied;
    }
    ImGui::SameLine();
    if(ImGui::Button("Ativar perfil Dev",MenuSize(190,36))) {
        ApplyProfile(1);
        const bool godmode_active=godmode_ready.load();
        std::snprintf(status,sizeof(status),godmode_active?
            "Perfil Dev ativo: Godmode e modificacoes do port ligados.":
            "Perfil Dev ativo, mas Godmode nao passou na validacao desta versao.");
        status_ok=godmode_active;
    }
    ImGui::InputText("Nome do perfil salvo",profile_name,sizeof(profile_name));
    ImGui::BeginDisabled(SelectedProfile()<2);
    if(ImGui::Button("Salvar opcoes atuais neste perfil",MenuSize(285,36))) {
        if(SaveUserProfile(SelectedProfile()-2,profile_name)) {
            app::Save();std::snprintf(status,sizeof(status),"Perfil salvo. As opcoes atuais foram guardadas.");
            status_ok=true;
        } else {std::snprintf(status,sizeof(status),"Digite um nome antes de salvar o perfil.");status_ok=false;}
    }
    ImGui::EndDisabled();
    if(status[0])ImGui::TextColored(status_ok?gold:crimson,"%s",status);
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if(ImGui::Button("Restaurar padrao vanilla",MenuSize(245,40))) {
        ApplyProfile(0);std::snprintf(status,sizeof(status),"Padroes restaurados. Voce pode alterar as opcoes agora.");
        status_ok=true;
    }
    ImGui::TextWrapped("O perfil vanilla restaura imagem, controles, audio, camera e opcoes do port aos valores originais. Seus perfis salvos permanecem disponiveis, e voce pode alterar qualquer opcao logo depois.");
    ImGui::TextWrapped("O perfil Dev liga Godmode, PT-BR, minimapa rotativo e HUD em 50%%. Mantem a camera original para evitar o modo orbital que voce relatou como bugado.");
}
void SavesPage() {
    Heading("Saves e sessao", "Arquivos de progresso do jogo. Carregamento usa o menu original.");
    ImGui::TextWrapped("Para carregar outro slot, reinicie no titulo e escolha Load Game. O progresso que ainda nao foi salvo sera perdido.");
    if(ImGui::Button("Reiniciar no titulo / carregar save")) ImGui::OpenPopup("Confirmar reinicio");
    if(ImGui::BeginPopupModal("Confirmar reinicio",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("O progresso nao salvo sera perdido. Seus saves gravados ficam preservados.");
        if(ImGui::Button("Reiniciar no titulo")) { app::Save();RequestTitleRestart();ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();if(ImGui::Button("Continuar jogando")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Arquivos locais em hdd (somente leitura)");
    static std::vector<std::string> files;
    static bool scanned=false;
    const bool refresh=ImGui::Button("Atualizar lista de arquivos");
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
        if(++count>=100){files.push_back("Lista limitada aos primeiros 100 arquivos.");break;}
    }
    }
    for(const auto& file:files) ImGui::TextWrapped("%s",file.c_str());
    if(files.empty()) ImGui::TextUnformatted("Nenhum arquivo de save encontrado.");
    ImGui::Spacing();
    ImGui::TextWrapped("Savestate completo ainda exige suporte do motor. Esses arquivos sao saves normais, nao um estado instantaneo da partida.");
}
void ModsPage() {
    Heading("Mods", "Arquivos adicionais, sem sobrescrever os assets originais.");
    if (ImGui::Button("Recarregar lista de mods")) ReloadMods();
    ImGui::TextWrapped("Coloque arquivos em mods/NomeDoMod/files/ com o mesmo caminho do jogo. Ative abaixo e reinicie para recarregar assets ja em cache.");
    ImGui::Spacing();
    if (Mods().empty()) ImGui::TextColored(muted_text, "Nenhum mod instalado. Consulte MODDING.md na pasta do projeto.");
    for (size_t i=0;i<Mods().size();i++) {
        bool enabled=Mods()[i].enabled;
        if (ImGui::Checkbox(Mods()[i].name.c_str(), &enabled)) EnableMod(i,enabled);
    }
    ImGui::Spacing();
    ImGui::TextWrapped("Se dois mods substituirem o mesmo arquivo, o nome em ordem alfabetica mais alta tem prioridade. So arquivos do disco do jogo sao substituidos; saves ficam fora dessa camada.");
    Toggle("Exportar texturas para criar um pack HD", texture_dump);
    ImGui::Text("Texturas exportadas: %u | Substituicoes HD carregadas: %u",texture_exports.load(),texture_replacements.load());
    ImGui::TextWrapped("Exporta ate 200 texturas por execucao para texture-dumps. Packs usam arquivos BMP nomeados pelo hash da textura; veja MODDING.md.");
}
void CheatsPage() {
    Heading("Cheats", "Patches de memoria para esta versao de Xbox, com valor esperado e reversao.");
    ImGui::TextWrapped("Codigos GameShark de PS2 nao sao compativeis. Use cheats.toml com enderecos validados do Xbox USA. Cheats nao sao ativados automaticamente.");
    if (ImGui::Button("Recarregar cheats.toml")) ReloadCheats();
    ImGui::Spacing();
    GameHacks();
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    if (Cheats().empty()) ImGui::TextColored(muted_text,"Nenhum perfil adicional em cheats.toml.");
    for (auto& cheat : Cheats()) {
        bool enabled=cheat.enabled;
        if (ImGui::Checkbox(cheat.name.c_str(), &enabled)) ToggleCheat(cheat,enabled);
        ImGui::SameLine(); ImGui::TextColored(muted_text,"%s",cheat.status.c_str());
    }
    ImGui::Spacing();
    ImGui::TextWrapped("Ao desligar um cheat, o valor original so e restaurado se a memoria ainda contiver o valor escrito pelo patch. A seguranca depende de validar o endereco e o contexto.");
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
    if (DrawCharacterSelection()) {DrawToast();return;}
    static bool was_open=false,editing_item=false;
    if (!menu_open) { if(was_open) app::Save();was_open=false;editing_item=false;DrawToast();return; }
    const bool was_editing=editing_item || ImGui::IsAnyItemActive();
    was_open=true;
    if (!CurrentReport().empty() && ReportStatus().find("Capturando") == 0) page=1;
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
    ImGui::SetNextWindowPos({std::clamp(menu_preferences.x,0.0f,std::max(0.0f,screen.x-160)),std::clamp(menu_preferences.y,0.0f,std::max(0.0f,screen.y-80))},condition);
    menu_geometry_reset=false;
    bool open=true;
    if (ImGui::Begin("Castlevania | Ferramentas", &open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
        const ImVec2 pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
        menu_preferences.x=pos.x;menu_preferences.y=pos.y;menu_preferences.width=size.x;menu_preferences.height=size.y;menu_preferences.positioned=true;
        ImGui::GetWindowDrawList()->AddRect({pos.x+6,pos.y+6},{pos.x+size.x-6,pos.y+size.y-6},IM_COL32(105,78,39,255));
        ImGui::PushFont(brand_font);ImGui::TextColored(gold,"Castlevania");ImGui::PopFont();
        ImGui::SameLine();ImGui::PushFont(section_font);ImGui::TextColored(crimson,"Curse of Darkness");ImGui::PopFont();
        char status[64];std::snprintf(status,sizeof(status),"%.0f FPS  |  %.2fx",fps,speed.load());
        ImGui::SameLine(std::max(ImGui::GetCursorPosX(),size.x-ImGui::CalcTextSize(status).x-24*scale));
        ImGui::TextColored(muted_text,"%s",status);
        ImGui::TextColored(muted_text,"Opcoes do port  /  Xbox USA");
        if(kernel::g_base) ImGui::TextColored(muted_text,"Mapa %d  /  Sala %d",
            int(static_cast<int8_t>(kernel::g_base[0xB234F4])),int(static_cast<int8_t>(kernel::g_base[0xB234F5])));
        ImGui::Separator();
        const float footer=54*scale;
        const char* sections[]{"Jogo","Imagem","Som","Controles","Mods e cheats","Sistema"};
        const int section_pages[]{0,2,3,4,6,8};
        int section=page<=1 ? 0 : page==2 ? 1 : page==3 ? 2 : page==4 ? 3 : page==5 || page==6 ? 4 : 5;
        bool switch_section=false;
        const int section_step=MenuSectionStep(was_editing,ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId));
        if(section_step) {section=(section+section_step+6)%6;switch_section=true;}
        if(ImGui::IsKeyPressed(ImGuiKey_GamepadL1)){section=(section+5)%6;switch_section=true;}
        if(ImGui::IsKeyPressed(ImGuiKey_GamepadR1)){section=(section+1)%6;switch_section=true;}
        const float tab_width=(ImGui::GetContentRegionAvail().x-5*8*scale)/6;
        for(int i=0;i<6;++i){
            if(i) ImGui::SameLine(0,8*scale);
            if(ImGui::Selectable(sections[i],section==i,0,{tab_width,34*scale})) {section=i;switch_section=true;}
        }
        if(switch_section) page=section_pages[section];
        ImGui::Separator();
        auto sub=[&](const char* label,int target){if(ImGui::Selectable(label,page==target,0,{190*scale,28*scale}))page=target;};
        if(section==0){sub("Ferramentas",0);ImGui::SameLine();sub("Relatar problema",1);}
        else if(section==4){sub("Cheats",6);ImGui::SameLine();sub("Packs e arquivos",5);}
        else if(section==5){sub("Saves e sessao",8);ImGui::SameLine();sub("Aparencia",7);ImGui::SameLine();sub("Perfis",9);}
        else ImGui::Dummy({0,28*scale});
        ImGui::BeginChild("content", {0,-footer});
        if(switch_section) {ImGui::SetScrollY(0);ImGui::SetKeyboardFocusHere();}
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x*.62f);
        switch(page) {case 0:General(fps);break;case 1:Reports();break;case 2:Graphics();break;
            case 3:Audio();break;case 4:Controls();break;case 5:ModsPage();break;case 6:CheatsPage();break;case 7:Appearance();break;case 8:SavesPage();break;case 9:ProfilesPage();break;}
        ImGui::PopItemWidth();ImGui::EndChild();
        ImGui::Separator();
        if(ImGui::Button("Salvar opcoes",{170*scale,36*scale})) app::Save();
        ImGui::SameLine();
        if(ImGui::Button("Voltar ao jogo",{185*scale,36*scale})) open=false;
        ImGui::SameLine();ImGui::TextColored(muted_text,"Esq/dir ou L1/R1: secao | A: ajustar | B: voltar");
        if(ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight) && !was_editing && !ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId)) open=false;
    }
    ImGui::End();
    editing_item=ImGui::IsAnyItemActive();
    if(!open) { menu_open.store(false); ShutdownInput(); app::Save();was_open=false; }
    DrawToast();
}
}
