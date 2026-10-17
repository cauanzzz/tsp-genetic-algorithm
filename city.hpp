#ifndef CIDADE_HPP
#define CIDADE_HPP

#include <cmath>
#include <vector>
#include <random>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Ponto
{
    double x;
    double y;

    double distanciaEntre(const Ponto& outro) const
    {
        double dx = x - outro.x;
        double dy = y - outro.y;
        return std::sqrt(dx*dx+dy*dy);
    }
};

enum class TipoCenario
{
    UNIFORME,
    CIRCULAR
};

class GeradorCenarios {
public:
    static std::vector<Ponto> gerarCidade( size_t numCidades, TipoCenario tipo, double largura = 800.0, double altura = 600.0, unsigned int seed = 42)
    {
        std::vector<Ponto> cidades;
        cidades.reserve(numCidades);

        if (tipo==TipoCenario::UNIFORME)
        {
            std::mt19937 rng(seed);
            std::uniform_real_distribution<double> distX(50.0, largura - 50.0);
            std::uniform_real_distribution<double> distY(50.0, altura - 50.0);

            for (size_t i=0; i<numCidades; ++i)
            {
                cidades.push_back({distX(rng),distY(rng)});
            }
        }
        else if (tipo == TipoCenario::CIRCULAR)
        {
            double centroX = largura/2.0;
            double centroY = altura/2.0;

            double raio = (std::min(largura,altura)/2.0) - 60.0;
            double passoAngular = (2.0*M_PI)/numCidades;

            for (size_t i=0;i<numCidades;++i)
            {
                double angulo=i*passoAngular;
                //conversao de cordenada polar (r e ang pra x e y)
                double x= centroX + raio * std::cos(angulo);
                double y= centroY + raio * std::sin(angulo);

                cidades.push_back({x,y});
            }
        }

        return cidades;
        
    }

    static std::vector<std::vector<double>> calcularMatrizDistancias(const std::vector<Ponto>& cidades)
    {
        size_t n = cidades.size();
        std::vector<std::vector<double>> matriz(n, std::vector<double>(n,0.0));

        for (size_t i=0; i<n; ++i)
        {
            for(size_t j=0; j<n; ++j)
            {
                matriz[i][j]=cidades[i].distanciaEntre(cidades[j]);
            }
        }
        return matriz;
    }
};

#endif
