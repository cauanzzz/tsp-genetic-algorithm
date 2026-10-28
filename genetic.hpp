#ifndef GENETICO_HPP
#define GENETICO_HPP

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
#include "city.hpp"

struct Individuo {
    std::vector<int> genoma;
    double distanciaTotal = 0.0;
    double fitness = 0.0;

    void calcularFitness(const std::vector<std::vector<double>>& matrizDistancias) {
        distanciaTotal = 0.0;
        const size_t n = genoma.size();

        for (size_t i = 0; i < n; ++i) {
            const int de = genoma[i];
            const int para = genoma[(i + 1) % n];
            distanciaTotal += matrizDistancias[de][para];
        }

        fitness = distanciaTotal > 0.0 ? 1.0 / distanciaTotal : 0.0;
    }
};

struct ParametrosAG {
    size_t tamanhoPopulacao = 100;
    size_t numGeracoes = 500;
    double taxaCrossover = 0.85;
    double taxaMutacao = 0.05;
    size_t tamanhoTorneio = 5;
    bool usarElitismo = true;
};

class AlgoritmoGenetico {
private:
    ParametrosAG params;
    std::vector<std::vector<double>> matrizDistancias;
    size_t numCidades;
    std::mt19937 rng;

public:
    AlgoritmoGenetico(const std::vector<Ponto>& cidades,
                     ParametrosAG parametros,
                     unsigned int seed = 42)
        : params(parametros), numCidades(cidades.size()), rng(seed) {
        matrizDistancias = GeradorCenarios::calcularMatrizDistancias(cidades);
    }

    std::vector<Individuo> gerarPopulacaoInicial() {
        std::vector<Individuo> populacao(params.tamanhoPopulacao);
        std::vector<int> rotaBase(numCidades);
        std::iota(rotaBase.begin(), rotaBase.end(), 0);

        for (auto& ind : populacao) {
            ind.genoma = rotaBase;
            std::shuffle(ind.genoma.begin(), ind.genoma.end(), rng);
            ind.calcularFitness(matrizDistancias);
        }

        return populacao;
    }

    Individuo selecaoTorneio(const std::vector<Individuo>& populacao) {
        if (populacao.empty()) {
            throw std::invalid_argument("A populacao nao pode estar vazia.");
        }

        std::uniform_int_distribution<size_t> dist(0, populacao.size() - 1);
        Individuo melhor = populacao[dist(rng)];

        for (size_t i = 1; i < params.tamanhoTorneio; ++i) {
            const Individuo& desafiante = populacao[dist(rng)];
            if (desafiante.fitness > melhor.fitness) {
                melhor = desafiante;
            }
        }

        return melhor;
    }

    Individuo crossoverOX(const Individuo& pai1, const Individuo& pai2) {
        if (numCidades < 2) {
            return pai1;
        }

        std::uniform_real_distribution<double> distProb(0.0, 1.0);
        if (distProb(rng) >= params.taxaCrossover) {
            return pai1;
        }

        Individuo filho;
        filho.genoma.assign(numCidades, -1);

        std::uniform_int_distribution<size_t> distIdx(0, numCidades - 1);
        size_t idx1 = distIdx(rng);
        size_t idx2 = distIdx(rng);
        if (idx1 > idx2) {
            std::swap(idx1, idx2);
        }

        std::vector<bool> presente(numCidades, false);
        for (size_t i = idx1; i <= idx2; ++i) {
            filho.genoma[i] = pai1.genoma[i];
            presente[pai1.genoma[i]] = true;
        }

        size_t posAtualFilho = (idx2 + 1) % numCidades;
        for (size_t i = 0; i < numCidades; ++i) {
            const size_t posPai2 = (idx2 + 1 + i) % numCidades;
            const int cidadePai2 = pai2.genoma[posPai2];

            if (!presente[cidadePai2]) {
                filho.genoma[posAtualFilho] = cidadePai2;
                posAtualFilho = (posAtualFilho + 1) % numCidades;
            }
        }

        filho.calcularFitness(matrizDistancias);
        return filho;
    }

    void mutacaoInversao(Individuo& ind) {
        if (numCidades < 2) {
            return;
        }

        std::uniform_real_distribution<double> distProb(0.0, 1.0);
        if (distProb(rng) < params.taxaMutacao) {
            std::uniform_int_distribution<size_t> distIdx(0, numCidades - 1);
            size_t idx1 = distIdx(rng);
            size_t idx2 = distIdx(rng);
            if (idx1 > idx2) {
                std::swap(idx1, idx2);
            }

            std::reverse(ind.genoma.begin() + idx1,
                         ind.genoma.begin() + idx2 + 1);
            ind.calcularFitness(matrizDistancias);
        }
    }

    std::vector<Individuo> evoluirGeracao(const std::vector<Individuo>& populacaoAtual) {
        if (populacaoAtual.empty() || params.tamanhoPopulacao == 0) {
            return {};
        }

        std::vector<Individuo> novaPopulacao;
        novaPopulacao.reserve(params.tamanhoPopulacao);

        if (params.usarElitismo) {
            const auto melhorIt = std::max_element(
                populacaoAtual.begin(), populacaoAtual.end(),
                [](const Individuo& a, const Individuo& b) {
                    return a.fitness < b.fitness;
                });
            novaPopulacao.push_back(*melhorIt);
        }

        while (novaPopulacao.size() < params.tamanhoPopulacao) {
            Individuo pai1 = selecaoTorneio(populacaoAtual);
            Individuo pai2 = selecaoTorneio(populacaoAtual);
            Individuo filho = crossoverOX(pai1, pai2);
            mutacaoInversao(filho);
            novaPopulacao.push_back(std::move(filho));
        }

        return novaPopulacao;
    }
};

#endif
