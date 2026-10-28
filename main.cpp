#include "raylib.h"
#include "city.hpp"
#include "genetic.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Atlas {
constexpr float pi = 3.14159265358979323846f;
const Color papel = {230, 214, 176, 255};
const Color tinta = {65, 48, 33, 255};
const Color suave = {125, 105, 76, 255};
const Color ouro = {179, 142, 76, 255};
const Color vinho = {147, 57, 39, 255};
const Color verde = {61, 88, 69, 255};
const Color creme = {238, 222, 185, 255};
const Color escuro = {43, 48, 39, 255};

struct Arte {
    Font titulo{}, corpo{}, italico{};
    Texture2D textura{};
    std::vector<Font> carregadas;

    Font fonte(const std::vector<const char*>& caminhos) {
        for (const char* caminho : caminhos) {
            if (!FileExists(caminho)) continue;
            Font f = LoadFontEx(caminho, 96, nullptr, 0);
            if (f.texture.id != 0 && f.texture.id != GetFontDefault().texture.id) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                carregadas.push_back(f);
                return f;
            }
        }
        return GetFontDefault();
    }
    Arte(int w, int h) {
        titulo = fonte({"C:/Windows/Fonts/georgia.ttf", "C:/Windows/Fonts/times.ttf",
                        "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf"});
        italico = fonte({"C:/Windows/Fonts/georgiai.ttf", "C:/Windows/Fonts/timesi.ttf",
                         "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Italic.ttf"});
        corpo = fonte({"C:/Windows/Fonts/calibri.ttf", "C:/Windows/Fonts/arial.ttf",
                       "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"});
        Image imagem = GenImageColor(w, h, papel);

        auto* pixels = static_cast<Color*>(imagem.data);
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            uint32_t hash = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
            hash = (hash ^ (hash >> 13)) * 1274126177u;
            const float ruido = float(hash % 1000u) / 1000.0f - .5f;
            const float dx = (float(x) - w * .52f) / (w * .65f);
            const float dy = (float(y) - h * .48f) / (h * .65f);
            const float borda = dx * dx + dy * dy;
            const float mancha = std::sin(x * .007f + std::sin(y * .008f))
                               * std::cos(y * .009f) * 3.5f;
            const float fibra = std::sin(y * 2.4f + x * .08f) * .7f;
            const float v = ruido * 11 + mancha + fibra - borda * 15;
            pixels[y * w + x] = {
                (unsigned char)std::clamp(235 + int(v), 0, 255),
                (unsigned char)std::clamp(219 + int(v), 0, 255),
                (unsigned char)std::clamp(182 + int(v), 0, 255), 255};
        }
        textura = LoadTextureFromImage(imagem);
        UnloadImage(imagem);
        SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    }
    void liberar() {
        UnloadTexture(textura);
        for (Font f : carregadas) UnloadFont(f);
    }
    void texto(const char* s, float x, float y, float tamanho, Color c,
               int estilo = 0, float espaco = 0) const {
        const Font f = estilo == 1 ? titulo : (estilo == 2 ? italico : corpo);
        DrawTextEx(f, s, {x, y}, tamanho, espaco, c);
    }
    float largura(const char* s, float tamanho, int estilo = 0, float espaco = 0) const {
        return MeasureTextEx(estilo == 1 ? titulo : (estilo == 2 ? italico : corpo),
                             s, tamanho, espaco).x;
    }
    void centro(const char* s, float x, float y, float tamanho, Color c, int estilo = 0) const {
        texto(s, x - largura(s, tamanho, estilo) / 2, y, tamanho, c, estilo);
    }
};

void linha(float x, float y, float w, Color cor, float grossura = 1) {
    DrawLineEx({x, y}, {x + w, y}, grossura, cor);
}
void losango(Vector2 c, float r, Color cor) {
    DrawTriangle({c.x, c.y-r}, {c.x-r, c.y}, {c.x, c.y+r}, cor);
    DrawTriangle({c.x, c.y-r}, {c.x, c.y+r}, {c.x+r, c.y}, cor);
}
void moldura(Rectangle r) {
    DrawRectangleLinesEx(r, 1, Fade(tinta, .65f));
    DrawRectangleLinesEx({r.x+5,r.y+5,r.width-10,r.height-10}, 1, Fade(tinta,.24f));
    for (float x : {r.x, r.x+r.width}) for (float y : {r.y,r.y+r.height}) {
        DrawCircle(int(x),int(y),3,ouro);
        DrawCircleLines(int(x),int(y),5,Fade(tinta,.6f));
    }
}

void rosa(const Arte& a, Vector2 c, float r) {
    DrawCircleLines(int(c.x),int(c.y),r*.66f,Fade(tinta,.45f));
    DrawCircleLines(int(c.x),int(c.y),r*.72f,Fade(tinta,.2f));
    for (int i=0;i<16;++i) {
        const float t = i*pi/8 - pi/2;
        const float comprimento = i%4 == 0 ? r : (i%2 == 0 ? r*.72f : r*.49f);
        const Vector2 ponta{c.x+std::cos(t)*comprimento,c.y+std::sin(t)*comprimento};
        const Vector2 b{c.x+std::cos(t+pi/2)*r*.11f,c.y+std::sin(t+pi/2)*r*.11f};
        const Vector2 d{c.x-std::cos(t+pi/2)*r*.11f,c.y-std::sin(t+pi/2)*r*.11f};
        DrawTriangle(c,b,ponta,Fade(i%2==0?tinta:ouro,.72f));
        DrawTriangle(c,ponta,d,Fade(i%2==0?ouro:tinta,.42f));
    }
    DrawCircleV(c,4,papel);
    DrawCircleLines(int(c.x),int(c.y),4,tinta);
    a.centro("N",c.x,c.y-r-21,17,tinta,1);
    a.centro("S",c.x,c.y+r+8,12,suave,1);
    a.centro("O",c.x-r-14,c.y-6,12,suave,1);
    a.centro("L",c.x+r+14,c.y-6,12,suave,1);
}

void relevo(Rectangle area) {
    BeginScissorMode(int(area.x),int(area.y),int(area.width),int(area.height));
    const Vector2 centros[]={{125,432},{906,591},{873,294}};
    const float raios[]={140,128,77};
    for (int k=0;k<3;++k) {
        for (int camada=0;camada<5;++camada) {
            Vector2 anterior{};
            for (int i=0;i<=150;++i) {
                const float t=i*2*pi/150;
                const float r=raios[k]+camada*6+15*std::sin(3*t+k)+10*std::sin(7*t+2*k);
                const Vector2 p{centros[k].x+std::cos(t)*r,
                                centros[k].y+std::sin(t)*r*.78f};
                if(i)DrawLineEx(anterior,p,camada==0?1.4f:.7f,
                                 Fade(verde,camada==0?.20f:.075f));
                anterior=p;
            }
        }
    }

    for(int i=0;i<14;++i) {
        const float x=101+float(i%4)*19+std::sin(float(i))*7;
        const float y=512+float(i/4)*25;
        const float h=14+float(i%3)*4;
        DrawLineEx({x-10,y},{x,y-h},1,Fade(suave,.25f));
        DrawLineEx({x,y-h},{x+13,y},1,Fade(suave,.25f));
        for(int j=0;j<4;++j)
            DrawLineEx({x+float(j)*2,y-h+float(j)*4},{x+float(j)*2+5,y},.6f,Fade(suave,.16f));
    }

    const Vector2 centro{902,725};
    for(int i=0;i<16;++i) {
        const float t=i*pi/8;
        DrawLineEx(centro,{centro.x+std::cos(t)*1100,centro.y+std::sin(t)*1100},
                   .7f,Fade(tinta,.045f));
    }
    EndScissorMode();
}

void cidade(Vector2 p, float tamanho, bool capital, bool foco) {
    const Color cor = capital ? vinho : tinta;
    DrawEllipse(int(p.x+2),int(p.y+4),tamanho*1.4f,tamanho*.6f,Fade(tinta,.12f));
    if(foco)DrawCircleLines(int(p.x),int(p.y),tamanho*2.7f,Fade(vinho,.75f));
    if(tamanho<4.5f) {
        DrawCircleV(p,tamanho+1,creme);
        DrawCircleV(p,tamanho*.65f,cor);
        return;
    }
    const float s=tamanho;
    DrawRectangleRec({p.x-s,p.y-s*.55f,s*2,s*1.2f},creme);
    DrawRectangleLinesEx({p.x-s,p.y-s*.55f,s*2,s*1.2f},.9f,cor);
    DrawTriangle({p.x-s-2,p.y-s*.55f},{p.x+s+2,p.y-s*.55f},{p.x,p.y-s*1.45f},cor);
    DrawRectangleRec({p.x-s*.18f,p.y-s*.1f,s*.36f,s*.75f},cor);
    if(capital) {
        DrawLineEx({p.x,p.y-s*1.45f},{p.x,p.y-s*2.6f},1,vinho);
        DrawTriangle({p.x,p.y-s*2.6f},{p.x,p.y-s*1.7f},{p.x+s,p.y-s*2.2f},vinho);
    }
}

void chave(const Arte& a,const char* k,const char* s,float y) {
    DrawRectangleRounded({1062,y,77,23},.15f,4,Fade(ouro,.15f));
    a.centro(k,1100,y+4,11,creme);
    a.texto(s,1150,y+3,14,Fade(creme,.82f));
}

void grafico(const Arte& a,const std::vector<double>& h,Rectangle area) {
    if(h.empty())return;
    auto mm=std::minmax_element(h.begin(),h.end());
    const double margem=std::max(1.0,(*mm.second-*mm.first)*.12);
    const double baixo=*mm.first-margem;
    const double delta=*mm.second-*mm.first+2*margem;
    for(int i=0;i<=3;++i)linha(area.x,area.y+area.height*i/3,area.width,Fade(creme,.10f));
    auto ponto=[&](size_t i)->Vector2 {
        return {area.x+(h.size()>1?float(i)/float(h.size()-1):0)*area.width,
                area.y+area.height*(1-float((h[i]-baixo)/delta))};
    };
    for(size_t i=1;i<h.size();++i)DrawLineEx(ponto(i-1),ponto(i),1.8f,ouro);
    DrawCircleV(ponto(h.size()-1),3,creme);
    a.texto("0",area.x,area.y+area.height+8,11,Fade(creme,.5f));
    const char* fim=TextFormat("%d",int(h.size()-1));
    a.texto(fim,area.x+area.width-a.largura(fim,11),area.y+area.height+8,11,Fade(creme,.5f));
}
}

namespace Bonus {
constexpr size_t quantidadeCidades = 100;
constexpr size_t limiteGeracoes = 10000;
constexpr size_t limiteSemMelhora = 1000;
constexpr size_t intervaloRegistros = 500;
constexpr int geracoesPorQuadro = 10;
constexpr unsigned int seed = 42;
constexpr double tolerancia = 1e-8;
using Relogio = std::chrono::steady_clock;

struct Registro {
    size_t geracao;
    double segundosAG;
    Individuo melhor;
    size_t ultimaMelhoria;
    double segundosUltimaMelhoria;
};

struct Teste {
    bool ativo = false;
    bool encerrado = false;
    bool estagnou = false;
    double segundosAG = 0;
    double segundosUltimaMelhoria = 0;
    double referenciaMelhoria = 0;
    size_t ultimaMelhoria = 0;
    int selecionado = -1;
    std::vector<Registro> registros;

    void reiniciar(const Individuo& inicial) {
        encerrado = false;
        estagnou = false;
        segundosAG = segundosUltimaMelhoria = 0;
        ultimaMelhoria = 0;
        referenciaMelhoria = inicial.distanciaTotal;
        selecionado = -1;
        registros.clear();
        if (ativo) registrar(0, inicial);
    }

    void registrar(size_t geracao, const Individuo& melhor) {
        if (registros.empty() || registros.back().geracao != geracao)
            registros.push_back({geracao, segundosAG, melhor,
                                 ultimaMelhoria, segundosUltimaMelhoria});
    }

    void observar(size_t geracao, const Individuo& melhor, double duracao) {
        segundosAG += duracao;
        if (referenciaMelhoria - melhor.distanciaTotal > tolerancia) {
            referenciaMelhoria = melhor.distanciaTotal;
            ultimaMelhoria = geracao;
            segundosUltimaMelhoria = segundosAG;
        }
        estagnou = geracao - ultimaMelhoria >= limiteSemMelhora;
        encerrado = estagnou || geracao >= limiteGeracoes;
        if (geracao == 100 || geracao % intervaloRegistros == 0 || encerrado)
            registrar(geracao, melhor);
    }

    void navegar(int direcao) {
        if (registros.empty()) return;
        if (selecionado < 0)
            selecionado = direcao < 0 ? int(registros.size()) - 1 : 0;
        else
            selecionado = std::clamp(selecionado + direcao, 0, int(registros.size()) - 1);
    }
};

double otimoCircular(const std::vector<Ponto>& cidades) {
    double soma = 0;
    for (size_t i = 0; i < cidades.size(); ++i)
        soma += cidades[i].distanciaEntre(cidades[(i + 1) % cidades.size()]);
    return soma;
}

}

int main() {
    constexpr int larguraJanela=1440, alturaJanela=900;
    constexpr int larguraMapa=850, alturaMapa=720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(larguraJanela,alturaJanela,"O Atlas das Rotas | TSP");
    SetTargetFPS(60);
    Atlas::Arte arte(larguraJanela,alturaJanela);

    size_t numCidades=16;
    TipoCenario cenarioAtual=TipoCenario::CIRCULAR;
    ParametrosAG paramsAG;
    paramsAG.tamanhoPopulacao=100;
    paramsAG.numGeracoes=1000;
    paramsAG.taxaCrossover=.85;
    paramsAG.taxaMutacao=.08;
    bool evoluindo=false;
    size_t geracaoAtual=0;
    std::vector<Ponto> cidades;
    std::vector<Individuo> populacao;
    std::unique_ptr<AlgoritmoGenetico> ag;
    Individuo melhorGlobal;
    double distanciaInicial=0, distanciaOtima=0;
    std::vector<double> historico;
    Bonus::Teste bonus;
    size_t cidadesAntesBonus=numCidades;
    TipoCenario cenarioAntesBonus=cenarioAtual;
    const size_t geracoesNormais=paramsAG.numGeracoes;
    bool mostrarNomes=true, mostrarRelevo=true;

    auto reiniciar=[&]() {
        cidades=GeradorCenarios::gerarCidade(numCidades,cenarioAtual,larguraMapa,alturaMapa,42);
        ag=std::make_unique<AlgoritmoGenetico>(cidades,paramsAG,42);
        populacao=ag->gerarPopulacaoInicial();
        melhorGlobal=*std::max_element(populacao.begin(),populacao.end(),
            [](const Individuo& a,const Individuo& b){return a.fitness<b.fitness;});
        distanciaInicial=melhorGlobal.distanciaTotal;
        geracaoAtual=0;
        evoluindo=false;
        historico.assign(1,distanciaInicial);
        bonus.reiniciar(melhorGlobal);
        distanciaOtima=cenarioAtual==TipoCenario::CIRCULAR?Bonus::otimoCircular(cidades):0;
    };
    reiniciar();
    const Rectangle mapa={42,160,960,676};
    const Rectangle campo={78,244,888,504};
    const float escala=std::min(campo.width/larguraMapa,campo.height/alturaMapa);
    const Vector2 origem={campo.x+(campo.width-larguraMapa*escala)/2,
                          campo.y+(campo.height-alturaMapa*escala)/2};
    auto posicao=[&](size_t i)->Vector2 {
        return {origem.x+float(cidades[i].x)*escala,origem.y+float(cidades[i].y)*escala};
    };
    const char* nomes[]={"Porto Bruma","Vila do Cedro","Santa Aurora","Vale Sereno",
        "Monte Claro","Costa Dourada","Pedra Alta","Vila das Rosas",
        "Porto do Sol","Sao Vicente","Serra Azul","Boa Esperanca",
        "Rio das Pedras","Vila do Farol","Ponta Verde","Nova Estrela"};

    while(!WindowShouldClose()) {
        if(IsKeyPressed(KEY_B)) {
            bonus.ativo=!bonus.ativo;
            if(bonus.ativo) {
                cidadesAntesBonus=numCidades;
                cenarioAntesBonus=cenarioAtual;
                numCidades=Bonus::quantidadeCidades;
                cenarioAtual=TipoCenario::CIRCULAR;
                paramsAG.numGeracoes=Bonus::limiteGeracoes;
            } else {
                numCidades=cidadesAntesBonus;
                cenarioAtual=cenarioAntesBonus;
                paramsAG.numGeracoes=geracoesNormais;
            }
            reiniciar();
        }
        if(IsKeyPressed(KEY_SPACE) && geracaoAtual<paramsAG.numGeracoes
           && !(bonus.ativo && bonus.encerrado)) {
            evoluindo=!evoluindo;
            if(evoluindo)bonus.selecionado=-1;
        }
        if(IsKeyPressed(KEY_R))reiniciar();
        if(IsKeyPressed(KEY_N))mostrarNomes=!mostrarNomes;
        if(IsKeyPressed(KEY_M))mostrarRelevo=!mostrarRelevo;
        if(!bonus.ativo) {
            if(IsKeyPressed(KEY_C)) {
                cenarioAtual=cenarioAtual==TipoCenario::CIRCULAR
                    ?TipoCenario::UNIFORME:TipoCenario::CIRCULAR;
                reiniciar();
            }
            if(IsKeyPressed(KEY_UP) && numCidades<100) {
                numCidades+=numCidades<20?2:10;
                reiniciar();
            }
            if(IsKeyPressed(KEY_DOWN) && numCidades>8) {
                numCidades-=numCidades<=20?2:10;
                reiniciar();
            }
        }
        if(bonus.ativo && !evoluindo) {
            if(IsKeyPressed(KEY_LEFT))bonus.navegar(-1);
            if(IsKeyPressed(KEY_RIGHT))bonus.navegar(1);
        }
        const int passos=bonus.ativo?Bonus::geracoesPorQuadro:1;
        for(int p=0;p<passos && evoluindo && geracaoAtual<paramsAG.numGeracoes;++p) {
            const auto inicio=Bonus::Relogio::now();
            populacao=ag->evoluirGeracao(populacao);
            ++geracaoAtual;
            const auto it=std::max_element(populacao.begin(),populacao.end(),
                [](const Individuo& a,const Individuo& b){return a.fitness<b.fitness;});
            if(it->fitness>melhorGlobal.fitness)melhorGlobal=*it;
            const double segundos=std::chrono::duration<double>(Bonus::Relogio::now()-inicio).count();
            historico.push_back(melhorGlobal.distanciaTotal);
            if(bonus.ativo) {
                bonus.observar(geracaoAtual,melhorGlobal,segundos);
                if(bonus.encerrado)evoluindo=false;
            }
            if(geracaoAtual>=paramsAG.numGeracoes)evoluindo=false;
        }
        const bool terminou=geracaoAtual>=paramsAG.numGeracoes || (bonus.ativo && bonus.encerrado);
        const Bonus::Registro* registro=bonus.ativo && bonus.selecionado>=0
            ?&bonus.registros[size_t(bonus.selecionado)]:nullptr;
        const Individuo& exibido=registro?registro->melhor:melhorGlobal;
        const double melhoria=distanciaInicial>0?(1-exibido.distanciaTotal/distanciaInicial)*100:0;
        const float progresso=paramsAG.numGeracoes>0
            ?std::clamp(float(geracaoAtual)/float(paramsAG.numGeracoes),0.0f,1.0f):1.0f;
        const char* status=terminou
            ?(bonus.ativo?(bonus.estagnou?"PARADA POR ESTAGNACAO":"LIMITE DE GERACOES"):"EXPEDICAO CONCLUIDA")
            :(evoluindo?"TRACANDO NOVOS CAMINHOS":"AGUARDANDO PARTIDA");
        int hover=-1;
        const Vector2 mouse=GetMousePosition();
        if(CheckCollisionPointRec(mouse,campo)) {
            float maisPerto=225;
            for(size_t i=0;i<cidades.size();++i) {
                const auto q=posicao(i);
                const float d=(mouse.x-q.x)*(mouse.x-q.x)+(mouse.y-q.y)*(mouse.y-q.y);
                if(d<maisPerto){maisPerto=d;hover=int(i);}
            }
        }

        BeginDrawing();
        ClearBackground(Atlas::papel);
        DrawTexture(arte.textura,0,0,WHITE);

        DrawRectangleLinesEx({18,18,1404,864},1,Fade(Atlas::tinta,.35f));
        DrawRectangleLinesEx({23,23,1394,854},1,Fade(Atlas::tinta,.12f));
        arte.texto("C A R T O G R A F I A   C O M P U T A C I O N A L",44,38,12,Atlas::suave);
        arte.texto("O Atlas das Rotas",42,61,56,Atlas::tinta,1);
        arte.texto("Uma viagem pelo problema do caixeiro viajante",46,125,18,Atlas::suave,2);
        arte.texto("CADERNO DE EXPEDICOES",1052,49,12,Atlas::suave,0,1.2f);
        arte.texto("Algoritmo genetico",1052,73,23,Atlas::tinta,1);
        arte.texto("Permutacoes, encontros e novos caminhos.",1052,104,13,Atlas::suave,2);

        for(int i=0;i<18;++i) {
            const float t=i*2*Atlas::pi/18;
            DrawCircle(int(1360+std::cos(t)*25),int(87+std::sin(t)*25),10,Atlas::vinho);
        }
        DrawCircle(1360,87,31,Atlas::vinho);
        DrawCircleLines(1360,87,26,Fade(Atlas::creme,.45f));
        DrawCircleLines(1360,87,22,Fade(Atlas::creme,.18f));
        arte.centro("AG",1360,73,23,Atlas::creme,1);
        Atlas::linha(42,151,1356,Fade(Atlas::tinta,.4f));

        DrawRectangleRec({mapa.x+5,mapa.y+6,mapa.width,mapa.height},Fade(Atlas::tinta,.12f));
        DrawTextureRec(arte.textura,{mapa.x,mapa.y,mapa.width,mapa.height},{mapa.x,mapa.y},WHITE);
        DrawRectangleRec(mapa,Fade(Atlas::creme,.16f));
        Atlas::moldura(mapa);
        arte.texto("TABULA ITINERARIA",70,179,12,Atlas::suave,0,1.6f);
        arte.centro(bonus.ativo?"A grande expedicao":
            (cenarioAtual==TipoCenario::CIRCULAR?"As cidades do anel":"Os caminhos do interior"),
            522,196,28,Atlas::tinta,1);
        arte.texto(TextFormat("%02d POVOACOES",int(numCidades)),826,181,12,Atlas::suave,0,.6f);
        Atlas::linha(75,234,894,Fade(Atlas::tinta,.22f));

        const Rectangle cartografia={61,246,921,520};
        if(mostrarRelevo)Atlas::relevo(cartografia);
        BeginScissorMode(61,246,921,520);

        for(int i=0;i<=8;++i) {
            const float x=83+i*109;
            DrawLineEx({x,252},{x,758},.7f,Fade(Atlas::tinta,.075f));
            arte.texto(TextFormat("%02d",i*10),x-7,251,9,Fade(Atlas::suave,.5f));
        }
        for(int i=0;i<=5;++i) {
            const float y=268+i*94;
            Atlas::linha(70,y,904,Fade(Atlas::tinta,.075f),.7f);
        }
        arte.texto("T E R R A",91,366,15,Fade(Atlas::suave,.35f),2);
        arte.texto("I N C O G N I T A",85,388,12,Fade(Atlas::suave,.35f),2);
        arte.texto("M A R   D E",827,461,13,Fade(Atlas::verde,.4f),2);
        arte.texto("P O S S I B I L I D A D E S",795,483,10,Fade(Atlas::verde,.4f),2);

        const size_t n=exibido.genoma.size();

        for(size_t i=0;i<n;++i) {
            const Vector2 a=posicao(size_t(exibido.genoma[i]));
            const Vector2 b=posicao(size_t(exibido.genoma[(i+1)%n]));
            DrawLineEx({a.x+1,a.y+1},{b.x+1,b.y+1},3.5f,Fade(Atlas::tinta,.09f));
            DrawLineEx(a,b,numCidades>30?1.4f:2.0f,Fade(Atlas::vinho,numCidades>30?.55f:.82f));
        }

        if(n && evoluindo) {
            const float t=float(GetTime()*.9);
            const size_t trecho=size_t(t)%n;
            const float f=t-std::floor(t);
            const Vector2 a=posicao(size_t(exibido.genoma[trecho]));
            const Vector2 b=posicao(size_t(exibido.genoma[(trecho+1)%n]));
            const Vector2 q={a.x+(b.x-a.x)*f,a.y+(b.y-a.y)*f};
            DrawCircleV(q,5,Atlas::creme);
            DrawCircleV(q,2.7f,Atlas::vinho);
        }
        for(size_t i=0;i<cidades.size();++i) {
            const Vector2 q=posicao(i);
            const float tamanho=numCidades>30?3.8f:6.5f;
            Atlas::cidade(q,tamanho,i==0,int(i)==hover);
        }
        std::vector<Rectangle> ocupados;
        if(mostrarNomes && numCidades<=20) {
            for(size_t i=0;i<cidades.size();++i) {
                if(int(i)==hover)continue;
                const auto q=posicao(i);
                const char* nome=TextFormat("%02d  %s",int(i),nomes[i%16]);
                const float w=arte.largura(nome,12,2)+10;
                Rectangle candidatos[]={{q.x+12,q.y-7,w,19},{q.x-w-12,q.y-7,w,19},
                    {q.x-w/2,q.y+14,w,19},{q.x-w/2,q.y-33,w,19}};
                for(Rectangle r:candidatos) {
                    if(r.x<campo.x || r.y<campo.y || r.x+r.width>campo.x+campo.width
                        || r.y+r.height>campo.y+campo.height)continue;
                    bool livre=true;
                    for(Rectangle usado:ocupados)if(CheckCollisionRecs(r,usado)){livre=false;break;}
                    for(size_t j=0;j<cidades.size() && livre;++j) {
                        const Vector2 p=posicao(j);
                        if(CheckCollisionCircleRec(p,11,r))livre=false;
                    }
                    if(!livre)continue;
                    DrawRectangleRec(r,Fade(Atlas::creme,.84f));
                    arte.texto(nome,r.x+5,r.y+2,12,Atlas::tinta,2);
                    ocupados.push_back(r);
                    break;
                }
            }
        }
        if(hover>=0) {
            const auto q=posicao(size_t(hover));
            const char* nome=TextFormat("%02d  %s",hover,nomes[size_t(hover)%16]);
            const float w=arte.largura(nome,17,1)+28;
            const float x=std::clamp(q.x-w/2,70.0f,970.0f-w);
            const float y=std::clamp(q.y-60,258.0f,709.0f);
            DrawRectangleRec({x+2,y+3,w,36},Fade(Atlas::tinta,.14f));
            DrawRectangleRec({x,y,w,36},Atlas::creme);
            DrawRectangleLinesEx({x,y,w,36},1,Atlas::ouro);
            arte.texto(nome,x+14,y+8,17,Atlas::tinta,1);
        }
        EndScissorMode();
        Atlas::rosa(arte,{914,702},35);

        const float comprimento=100*escala;
        arte.texto("100 unidades",79,723,11,Atlas::suave,2);
        Atlas::linha(79,744,comprimento,Atlas::tinta,2);
        DrawLine(79,740,79,748,Atlas::tinta);
        DrawLine(int(79+comprimento),740,int(79+comprimento),748,Atlas::tinta);
        Atlas::linha(75,771,894,Fade(Atlas::tinta,.25f));
        if(bonus.ativo) {
            arte.texto(TextFormat("Tempo AG: %.3f s  |  Ultima melhoria: g%d, %.3f s",
                bonus.segundosAG,int(bonus.ultimaMelhoria),bonus.segundosUltimaMelhoria),
                77,783,14,Atlas::tinta);
            arte.texto(registro?TextFormat("ARQUIVO DE ROTAS  /  Geracao %d  /  ESQ e DIR para consultar",
                int(registro->geracao)):"Parada: 1000 geracoes sem melhora ou limite de 10000.",
                77,808,12,Atlas::suave,2);
        } else {
            arte.texto("Um circuito. Todas as cidades. O menor caminho possivel.",77,785,16,Atlas::tinta,2);
            arte.texto("B  /  GRANDE EXPEDICAO: TESTE BONUS COM 100 CIDADES",77,811,11,Atlas::vinho,0,.6f);
        }

        DrawRectangleRec({1033,165,370,676},Fade(Atlas::tinta,.16f));
        DrawRectangleRec({1028,160,370,676},Atlas::escuro);
        for(int i=0;i<120;++i)
            Atlas::linha(1033,165+i*5.5f,360,Fade(Atlas::creme,.012f));
        DrawRectangleLinesEx({1035,167,356,662},1,Fade(Atlas::ouro,.45f));
        arte.texto("D I A R I O   D E   B O R D O",1060,184,12,Atlas::ouro);
        arte.texto(bonus.ativo?"A grande expedicao":"A expedicao",1059,207,31,Atlas::creme,1);
        DrawCircle(1065,265,3,evoluindo?Atlas::ouro:Fade(Atlas::creme,.5f));
        arte.texto(status,1078,258,11,Atlas::creme,0,.5f);
        Atlas::linha(1060,289,306,Fade(Atlas::ouro,.36f));
        arte.texto(registro?"PERCURSO DO REGISTRO":"MELHOR PERCURSO",1060,307,11,Atlas::ouro,0,1.3f);
        arte.texto(TextFormat("%.1f",exibido.distanciaTotal),1057,329,52,Atlas::creme,1);
        arte.texto("unidades do mapa original",1061,390,13,Fade(Atlas::creme,.52f),2);
        arte.texto(TextFormat("Partida: %.1f",distanciaInicial),1060,418,15,Fade(Atlas::creme,.72f));
        arte.texto(TextFormat("%.2f%% de caminho economizado",melhoria),1060,446,16,Atlas::ouro);
        Atlas::linha(1060,480,306,Fade(Atlas::ouro,.36f));
        arte.texto("A EVOLUCAO DO PERCURSO",1060,498,11,Atlas::ouro,0,1);
        Atlas::grafico(arte,historico,{1060,530,306,73});
        arte.texto(TextFormat("Geracao %d / %d",int(geracaoAtual),int(paramsAG.numGeracoes)),
            1060,638,14,Atlas::creme);
        Atlas::linha(1060,664,306,Fade(Atlas::creme,.15f),2);
        if(progresso>0)Atlas::linha(1060,664,306*progresso,Atlas::ouro,2);
        Atlas::chave(arte,"ESPACO","Iniciar / pausar",684);
        Atlas::chave(arte,"R","Reiniciar a expedicao",714);
        Atlas::chave(arte,bonus.ativo?"ESQ / DIR":"C",bonus.ativo?"Registros (pausado)":"Trocar o cenario",744);
        Atlas::chave(arte,bonus.ativo?"B":"CIMA / BAIXO",bonus.ativo?"Voltar ao modo normal":"Quantidade de cidades",774);
        arte.texto("N: nomes   /   M: relevo",1060,809,11,Fade(Atlas::creme,.5f),2);

        arte.texto("TSP  /  ESTUDO DE ROTAS POR ALGORITMO GENETICO",44,855,11,Atlas::suave,0,.5f);
        const char* nota=distanciaOtima>0
            ?TextFormat("Otimo circular: %.2f  |  Excesso: %.2f%%",distanciaOtima,
                std::max(0.0,(exibido.distanciaTotal/distanciaOtima-1)*100))
            :"Relevo ilustrativo. O custo usa distancia euclidiana.";
        arte.texto(nota,1397-arte.largura(nota,12,2),853,12,Atlas::suave,2);
        EndDrawing();
    }
    arte.liberar();
    CloseWindow();
    return 0;
}
