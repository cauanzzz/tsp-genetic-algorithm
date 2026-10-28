#ifndef GENETICO_HPP
#define GENETICO_HPP

#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>
#include "city.hpp"

struct Individuo
{
    std::vector<int> genoma;
    double distanciaTotal;
    double fitness;

    void calcularFitness(const std::vector<std::vector<double>>& matrizDistancias)
    {
        distanciaTotal = 0.0;
        size_t n = genoma.size();

        for (size_t i=0;i<n;++i)
        {
            int de=genoma[i];
            int para=genoma[(i+1)%n];
            distanciaTotal+=matrizDistancias[de][para];
        }

        fitness=(distanciaTotal>0.0) ? (1.0/distanciaTotal) : 0.0;
    }
};

struct ParametrosAG
{
    size_t tamanhoPopulacao = 100;
    size_t numGeracoes = 500;
    double taxaCrossover = 0.85;
    double taxaMutacao = 0.05;
    size_t tamanhoTorneio = 5;
    bool usarElitismo = true;
};

class AlgoritmoGenetico
{
    private:
    ParametrosAG params;
    std::vector<std::vector<double>> matrizDistancias;
    size_t numCidades;
    std::mt19937 rng;

    public:
    AlgoritmoGenetico(const std::vector<Ponto>& cidades, ParametrosAG parametros, unsigned int seed = 42)
    : params(parametros), rng(seed) 
    {   
        numCidades=cidades.size();
        matrizDistancias=GeradorCenarios::calcularMatrizDistancias(cidades);
    }

    std::vector<Individuo> gerarPopulacaoInicial()
    {
        std::vector<Individuo> populacao(params.tamanhoPopulacao);
        std::vector<int> rotaBase(numCidades);
        std::iota(rotaBase.begin(),rotaBase.end(),0);

        for(auto& ind : populacao)
        {
            ind.genoma = rotaBase;
            std::shuffle(ind.genoma.begin(), ind.genoma.end(),rng);
            ind.calcularFitness(matrizDistancias);
        }
        return populacao;
    }

    Individuo selecaoTorneio(const std::vector<Individuo>& populacao)
    {
        std::uniform_int_distribution<size_t> dist(0,populacao.size()-1);
        Individuo melhor = populacao[dist(rng)];

        for(size_t i=1; i<params.tamanhoTorneio;i++)
        {
            Individuo desafiante=populacao[dist(rng)];
            if (desafiante.fitness>melhor.fitness)
            {
                melhor=desafiante;
            }
        }
        return melhor;
    }

    std::vector<Individuo> evoluirGeracao(const std::vector<Individuo>& populacao)
    {
        if (populacao.empty()) return {};

        std::vector<Individuo> novaPopulacao;
        novaPopulacao.reserve(params.tamanhoPopulacao);
        std::uniform_real_distribution<double> probabilidade(0.0, 1.0);

        if (params.usarElitismo && params.tamanhoPopulacao > 0)
        {
            novaPopulacao.push_back(*std::max_element(populacao.begin(), populacao.end(),
                [](const Individuo& a, const Individuo& b) { return a.fitness < b.fitness; }));
        }

        while (novaPopulacao.size() < params.tamanhoPopulacao)
        {
            Individuo filho = selecaoTorneio(populacao);
            if (numCidades > 1)
            {
                std::uniform_int_distribution<size_t> indice(0, numCidades - 1);
                if (probabilidade(rng) < params.taxaCrossover)
                {
                    const Individuo outroPai = selecaoTorneio(populacao);
                    size_t inicio = indice(rng);
                    size_t fim = indice(rng);
                    if (inicio > fim) std::swap(inicio, fim);

                    // Order crossover: preserva um trecho e completa sem repetir cidades.
                    std::vector<int> rota(numCidades, -1);
                    std::vector<bool> usadas(numCidades, false);
                    for (size_t i = inicio; i <= fim; ++i)
                    {
                        rota[i] = filho.genoma[i];
                        usadas[rota[i]] = true;
                    }
                    size_t destino = (fim + 1) % numCidades;
                    for (size_t i = 0; i < numCidades; ++i)
                    {
                        int cidade = outroPai.genoma[(fim + 1 + i) % numCidades];
                        if (!usadas[cidade])
                        {
                            rota[destino] = cidade;
                            destino = (destino + 1) % numCidades;
                        }
                    }
                    filho.genoma = std::move(rota);
                }
                if (probabilidade(rng) < params.taxaMutacao)
                {
                    size_t a = indice(rng);
                    size_t b = indice(rng);
                    std::swap(filho.genoma[a], filho.genoma[b]);
                }
            }
            filho.calcularFitness(matrizDistancias);
            novaPopulacao.push_back(std::move(filho));
        }
        return novaPopulacao;
    }

};

#endif

