#include "raylib.h"
#include "city.hpp"
#include "genetic.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <chrono>

namespace UI {
const Color fundo = {12, 18, 30, 255};
const Color superficie = {20, 29, 45, 255};
const Color elevado = {29, 41, 59, 255};
const Color texto = {231, 238, 247, 255};
const Color secundario = {141, 159, 183, 255};
const Color grade = {31, 43, 61, 255};
const Color destaque = {64, 214, 182, 255};
const Color azul = {100, 175, 255, 255};
const Color amarelo = {255, 201, 107, 255};

void cartao(Rectangle area) {
    DrawRectangleRounded({area.x, area.y + 4, area.width, area.height},
                         0.07f, 8, Fade(BLACK, 0.16f));
    DrawRectangleRounded(area, 0.07f, 8, superficie);
}

void atalho(const char* tecla, const char* descricao, int x, int y) {
    DrawRectangleRounded({float(x), float(y), 68, 21}, 0.2f, 4, elevado);
    DrawText(tecla, x + (68 - MeasureText(tecla, 10)) / 2, y + 5, 10, texto);
    DrawText(descricao, x + 80, y + 4, 12, secundario);
}

void grafico(const std::vector<double>& historico, Rectangle area) {
    if (historico.empty()) return;
    const auto limites = std::minmax_element(historico.begin(), historico.end());
    const double minimo = *limites.first;
    const double maximo = *limites.second;
    const double margem = std::max(1.0, (maximo - minimo) * 0.12);
    const double base = minimo - margem;
    const double amplitude = maximo - minimo + 2.0 * margem;

    for (int i = 0; i <= 3; ++i) {
        const float y = area.y + area.height * float(i) / 3.0f;
        DrawLineEx({area.x, y}, {area.x + area.width, y}, 1, grade);
    }
    auto posicao = [&](size_t i) -> Vector2 {
        const float t = historico.size() > 1
            ? float(i) / float(historico.size() - 1) : 0.0f;
        return {area.x + t * area.width,
                area.y + area.height * (1.0f - float((historico[i] - base) / amplitude))};
    };
    for (size_t i = 1; i < historico.size(); ++i)
        DrawLineEx(posicao(i - 1), posicao(i), 2.0f, destaque);
    DrawCircleV(posicao(historico.size() - 1), 3, destaque);
}
}

namespace Bonus {
constexpr size_t quantidadeCidades = 100;
constexpr size_t limiteGeracoes = 10000;
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
    bool atingiuOtimo = false;
    double distanciaOtima = 0.0;
    double segundosAG = 0;
    double segundosUltimaMelhoria = 0;
    double referenciaMelhoria = 0;
    size_t ultimaMelhoria = 0;
    int selecionado = -1;
    std::vector<Registro> registros;

    bool alcancouOtimo(const Individuo& individuo) const {
        return distanciaOtima > 0.0
            && std::abs(individuo.distanciaTotal - distanciaOtima)
                <= std::max(1.0, distanciaOtima) * 1e-9;
    }

    void reiniciar(const Individuo& inicial, double otimo = 0.0) {
        distanciaOtima = otimo;
        atingiuOtimo = alcancouOtimo(inicial);
        encerrado = ativo && atingiuOtimo;
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
        atingiuOtimo = alcancouOtimo(melhor);
        encerrado = atingiuOtimo || geracao >= limiteGeracoes;
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
    const int larguraJanela = 1200;
    const int alturaJanela = 720;
    const int larguraMapa = 850;

    InitWindow(larguraJanela, alturaJanela, "TSP | Laboratorio de rotas");
    SetTargetFPS(60);

    size_t numCidades = 16;
    TipoCenario cenarioAtual = TipoCenario::CIRCULAR;
    ParametrosAG paramsAG;
    paramsAG.tamanhoPopulacao = 100;
    paramsAG.numGeracoes = 1000;
    paramsAG.taxaCrossover = 0.85;
    paramsAG.taxaMutacao = 0.08;

    bool evoluindo = false;
    size_t geracaoAtual = 0;
    std::vector<Ponto> cidades;
    std::unique_ptr<AlgoritmoGenetico> ag;
    std::vector<Individuo> populacao;
    Individuo melhorGlobal;
    double distanciaInicial = 0.0;
    std::vector<double> historico;
    Bonus::Teste bonus;
    size_t cidadesAntesBonus = numCidades;
    TipoCenario cenarioAntesBonus = cenarioAtual;
    const size_t geracoesNormais = paramsAG.numGeracoes;
    double distanciaOtima = 0;

    auto reiniciarSimulacao = [&]() {
        cidades = GeradorCenarios::gerarCidade(
            numCidades, cenarioAtual, larguraMapa, alturaJanela);
        ag = std::make_unique<AlgoritmoGenetico>(cidades, paramsAG, Bonus::seed);
        populacao = ag->gerarPopulacaoInicial();
        melhorGlobal = *std::max_element(populacao.begin(), populacao.end(),
            [](const Individuo& a, const Individuo& b) { return a.fitness < b.fitness; });
        distanciaInicial = melhorGlobal.distanciaTotal;
        geracaoAtual = 0;
        evoluindo = false;
        historico.assign(1, distanciaInicial);
        distanciaOtima = bonus.ativo ? Bonus::otimoCircular(cidades) : 0;
        bonus.reiniciar(melhorGlobal, distanciaOtima);
    };
    reiniciarSimulacao();

    const Rectangle areaMapa = {24, 100, 802, 596};
    const Rectangle areaDesenho = {48, 164, 754, 474};
    const float escala = std::min(areaDesenho.width / float(larguraMapa),
                                  areaDesenho.height / float(alturaJanela));
    const Vector2 origem = {
        areaDesenho.x + (areaDesenho.width - larguraMapa * escala) / 2,
        areaDesenho.y + (areaDesenho.height - alturaJanela * escala) / 2
    };
    auto posicaoTela = [&](size_t indice) -> Vector2 {
        return {origem.x + float(cidades[indice].x) * escala,
                origem.y + float(cidades[indice].y) * escala};
    };

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_B)) {
            bonus.ativo = !bonus.ativo;
            if (bonus.ativo) {
                cidadesAntesBonus = numCidades;
                cenarioAntesBonus = cenarioAtual;
                numCidades = Bonus::quantidadeCidades;
                cenarioAtual = TipoCenario::CIRCULAR;
                paramsAG.numGeracoes = Bonus::limiteGeracoes;
            } else {
                numCidades = cidadesAntesBonus;
                cenarioAtual = cenarioAntesBonus;
                paramsAG.numGeracoes = geracoesNormais;
            }
            reiniciarSimulacao();
        }
        if (IsKeyPressed(KEY_SPACE) && geracaoAtual < paramsAG.numGeracoes
            && !(bonus.ativo && bonus.encerrado)) {
            evoluindo = !evoluindo;
            if (evoluindo) bonus.selecionado = -1;
        }
        if (IsKeyPressed(KEY_R)) reiniciarSimulacao();
        if (!bonus.ativo && IsKeyPressed(KEY_C)) {
            cenarioAtual = cenarioAtual == TipoCenario::CIRCULAR
                ? TipoCenario::UNIFORME : TipoCenario::CIRCULAR;
            reiniciarSimulacao();
        }
        if (!bonus.ativo && IsKeyPressed(KEY_UP) && numCidades < 100) {
            numCidades += numCidades < 20 ? 2 : 10;
            reiniciarSimulacao();
        }
        if (!bonus.ativo && IsKeyPressed(KEY_DOWN) && numCidades > 8) {
            numCidades -= numCidades <= 20 ? 2 : 10;
            reiniciarSimulacao();
        }
        if (bonus.ativo && !evoluindo) {
            if (IsKeyPressed(KEY_LEFT)) bonus.navegar(-1);
            if (IsKeyPressed(KEY_RIGHT)) bonus.navegar(1);
        }

        const int passos = bonus.ativo ? Bonus::geracoesPorQuadro : 1;
        for (int passo = 0; passo < passos && evoluindo
             && geracaoAtual < paramsAG.numGeracoes; ++passo) {
            const auto inicio = Bonus::Relogio::now();
            populacao = ag->evoluirGeracao(populacao);
            ++geracaoAtual;
            const auto melhorDaGeracao = std::max_element(populacao.begin(), populacao.end(),
                [](const Individuo& a, const Individuo& b) { return a.fitness < b.fitness; });
            if (melhorDaGeracao->fitness > melhorGlobal.fitness)
                melhorGlobal = *melhorDaGeracao;
            const double duracao = std::chrono::duration<double>(
                Bonus::Relogio::now() - inicio).count();
            historico.push_back(melhorGlobal.distanciaTotal);
            if (bonus.ativo) {
                bonus.observar(geracaoAtual, melhorGlobal, duracao);
                if (bonus.encerrado) evoluindo = false;
            }
            if (geracaoAtual >= paramsAG.numGeracoes) evoluindo = false;
        }
        const bool concluido = geracaoAtual >= paramsAG.numGeracoes
            || (bonus.ativo && bonus.encerrado);
        const Color corStatus = concluido ? UI::azul
            : (evoluindo ? UI::destaque : UI::amarelo);
        const char* status = concluido
            ? (bonus.ativo ? (bonus.atingiuOtimo ? "OTIMO ATINGIDO" : "LIMITE ATINGIDO") : "CONCLUIDO")
            : (evoluindo ? "EM EVOLUCAO" : "PAUSADO");
        const Bonus::Registro* registro = bonus.ativo && bonus.selecionado >= 0
            ? &bonus.registros[size_t(bonus.selecionado)] : nullptr;
        const Individuo& exibido = registro ? registro->melhor : melhorGlobal;
        const float progresso = paramsAG.numGeracoes > 0
            ? std::clamp(float(geracaoAtual) / float(paramsAG.numGeracoes), 0.0f, 1.0f)
            : 1.0f;
        const double melhora = distanciaInicial > 0
            ? (distanciaInicial - exibido.distanciaTotal) / distanciaInicial * 100.0 : 0;

        BeginDrawing();
        ClearBackground(UI::fundo);

        DrawRectangleRounded({24, 28, 44, 44}, 0.2f, 6, UI::elevado);
        DrawLineEx({35, 58}, {45, 40}, 2, UI::destaque);
        DrawLineEx({45, 40}, {58, 57}, 2, UI::destaque);
        DrawLineEx({58, 57}, {35, 58}, 2, UI::destaque);
        DrawCircle(35, 58, 3, UI::texto);
        DrawCircle(45, 40, 3, UI::texto);
        DrawCircle(58, 57, 3, UI::texto);
        DrawText("Laboratorio de rotas", 84, 26, 28, UI::texto);
        DrawText("PROBLEMA DO CAIXEIRO VIAJANTE  /  ALGORITMO GENETICO",
                 85, 62, 12, UI::secundario);
        DrawRectangleRounded({1020, 36, 156, 29}, 0.5f, 8, UI::elevado);
        DrawCircle(1034, 50, 4, corStatus);
        DrawText(status, 1046, 44, 12, corStatus);

        UI::cartao(areaMapa);
        DrawText(bonus.ativo ? "Teste bonus | Circular" : "Melhor rota encontrada",
                 46, 122, 20, UI::texto);
        if (registro)
            DrawText(TextFormat("Registro: geracao %d", int(registro->geracao)),
                     46, 148, 12, UI::amarelo);
        DrawText(TextFormat("%d CIDADES", int(numCidades)), 700, 128, 12, UI::secundario);

        // Recorte tambem protege o painel se houver pontos fora da area esperada.
        BeginScissorMode(int(areaDesenho.x), int(areaDesenho.y),
                         int(areaDesenho.width), int(areaDesenho.height));
        for (int x = int(areaDesenho.x); x < int(areaDesenho.x + areaDesenho.width); x += 32)
            for (int y = int(areaDesenho.y); y < int(areaDesenho.y + areaDesenho.height); y += 32)
                DrawCircle(x, y, 1, UI::grade);

        const size_t n = exibido.genoma.size();
        for (size_t i = 0; i < n; ++i) {
            const Vector2 a = posicaoTela(size_t(exibido.genoma[i]));
            const Vector2 b = posicaoTela(size_t(exibido.genoma[(i + 1) % n]));
            DrawLineEx(a, b, 7, Fade(UI::destaque, 0.07f));
            DrawLineEx(a, b, 2, Fade(UI::destaque, 0.85f));
        }

        int cidadeHover = -1;
        const Vector2 mouse = GetMousePosition();
        const bool mouseNoMapa = CheckCollisionPointRec(mouse, areaDesenho);
        for (size_t i = 0; i < cidades.size(); ++i) {
            const Vector2 p = posicaoTela(i);
            const float dx = mouse.x - p.x, dy = mouse.y - p.y;
            if (mouseNoMapa && dx * dx + dy * dy <= 144) cidadeHover = int(i);
            DrawCircleV(p, 11, Fade(UI::azul, 0.10f));
            DrawCircleV(p, 6, UI::fundo);
            DrawCircleV(p, 4, UI::azul);
            if (numCidades <= 30) {
                const char* rotulo = TextFormat("%d", int(i));
                DrawText(rotulo, int(p.x) - MeasureText(rotulo, 12) / 2,
                         int(p.y) - 24, 12, UI::secundario);
            }
        }
        if (cidadeHover >= 0) {
            const Vector2 p = posicaoTela(size_t(cidadeHover));
            DrawCircleLines(int(p.x), int(p.y), 10, UI::texto);
            const char* rotulo = TextFormat("Cidade %d", cidadeHover);
            const float largura = float(MeasureText(rotulo, 14) + 20);
            const float x = std::clamp(p.x + 14, areaDesenho.x,
                                      areaDesenho.x + areaDesenho.width - largura);
            const float y = std::clamp(p.y - 34, areaDesenho.y,
                                      areaDesenho.y + areaDesenho.height - 27);
            DrawRectangleRounded({x, y, largura, 27}, 0.15f, 4, UI::elevado);
            DrawText(rotulo, int(x) + 10, int(y) + 6, 14, UI::texto);
        }
        EndScissorMode();
        if (bonus.ativo) {
            const size_t epocaExibida = registro ? registro->geracao : geracaoAtual;
            const double tempoExibido = registro ? registro->segundosAG : bonus.segundosAG;
            const size_t ultimaMelhoria = registro ? registro->ultimaMelhoria : bonus.ultimaMelhoria;
            const double tempoUltimaMelhoria = registro
                ? registro->segundosUltimaMelhoria : bonus.segundosUltimaMelhoria;
            DrawText(TextFormat("Ate a epoca %d: %.4f s | Ultima melhoria: g%d (%.4f s)",
                     int(epocaExibida), tempoExibido, int(ultimaMelhoria), tempoUltimaMelhoria),
                     46, 646, 14, UI::texto);
            DrawText("Parada: otimo atingido ou limite de 10000 geracoes. R reinicia.",
                     46, 672, 12, UI::secundario);
        } else {
            DrawCircle(49, 670, 4, UI::destaque);
            DrawText("Melhor percurso", 62, 664, 12, UI::secundario);
            DrawCircle(215, 670, 4, UI::azul);
            DrawText("Cidades", 228, 664, 12, UI::secundario);
            DrawText("B: abrir teste bonus", 592, 664, 14, UI::amarelo);
        }

        UI::cartao({850, 100, 326, 112});
        DrawText(bonus.ativo
            ? TextFormat("SEM MELHORA: %d geracoes", int(geracaoAtual - bonus.ultimaMelhoria))
            : "SIMULACAO", 870, 118, 12, UI::secundario);
        DrawText(TextFormat("Geracao %d", int(geracaoAtual)), 870, 142, 22, UI::texto);
        DrawText(TextFormat("/ %d", int(paramsAG.numGeracoes)), 1050, 148, 14, UI::secundario);
        DrawRectangleRounded({870, 181, 286, 7}, 0.8f, 6, UI::elevado);
        if (progresso > 0)
            DrawRectangleRec({870, 181, 286 * progresso, 7}, UI::destaque);

        UI::cartao({850, 224, 326, 176});
        DrawText(registro ? "DISTANCIA DO REGISTRO" : "MELHOR DISTANCIA", 870, 243, 12, UI::secundario);
        DrawText(TextFormat("%.1f", exibido.distanciaTotal), 870, 267, 34, UI::texto);
        DrawText("unidades do mapa original", 871, 307, 12, UI::secundario);
        if (bonus.ativo) {
            DrawText(TextFormat("Otimo circular: %.2f", distanciaOtima), 870, 336, 14, UI::secundario);
            DrawText(TextFormat("Acima do otimo: %.2f%%",
                     std::max(0.0, (exibido.distanciaTotal / distanciaOtima - 1) * 100)),
                     870, 365, 16, UI::destaque);
        } else {
            DrawText(TextFormat("Inicial: %.1f", distanciaInicial), 870, 336, 14, UI::secundario);
            DrawText(TextFormat("Reducao de %.2f%%", melhora), 870, 365, 18, UI::destaque);
        }

        UI::cartao({850, 412, 326, 142});
        DrawText("MELHOR DISTANCIA POR GERACAO", 870, 430, 12, UI::secundario);
        UI::grafico(historico, {872, 458, 280, 63});
        DrawText("0", 870, 534, 10, UI::secundario);
        const char* ultima = TextFormat("%d", int(geracaoAtual));
        DrawText(ultima, 1152 - MeasureText(ultima, 10), 534, 10, UI::secundario);

        UI::cartao({850, 566, 326, 130});
        if (bonus.ativo) {
            DrawText("CONTROLES DO TESTE BONUS", 870, 580, 12, UI::azul);
            UI::atalho("ESPACO", "Iniciar / pausar", 870, 600);
            UI::atalho("ESQ/DIR", "Registros (pausado)", 870, 623);
            UI::atalho("R", "Reiniciar teste", 870, 646);
            UI::atalho("B", "Voltar ao modo normal", 870, 669);
        } else {
            DrawText(cenarioAtual == TipoCenario::CIRCULAR ? "CENARIO CIRCULAR" : "CENARIO UNIFORME",
                     870, 580, 12, UI::azul);
            UI::atalho("ESPACO", "Iniciar / pausar", 870, 600);
            UI::atalho("R", "Reiniciar", 870, 623);
            UI::atalho("C", "Trocar cenario", 870, 646);
            UI::atalho("CIMA/BAIXO", "Quantidade de cidades", 870, 669);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
