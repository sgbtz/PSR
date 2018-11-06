/*************************************************************************/
/**		PRÁCTICA 1 DE PLANIFICACIÓN Y SIMULACIÓN DE REDES	**/
/**								      	**/
/**			SIMULACION.CC					**/
/**								     	**/
/*************************************************************************/

#include <ns3/core-module.h>
#include <ns3/gnuplot.h>

#include "llegadas.h"

#define MEDIA    10
#define MUESTRAS 20
#define FIN      1000
// Valor de la t-Student para el 90% y 20 muestras
#define TSTUDENT 1.729

using namespace ns3;

// Se declara el nombre del registro (componente de trazado) para este módulo de código.
// En este caso el identificador será "Simulacion"
NS_LOG_COMPONENT_DEFINE ("Simulacion");

int main (int argc, char *argv[])
{
  // Se establece resolución de ms
  Time::SetResolution (Time::MS);
  // Se declara un objeto Time que guardará el tiempo de duración de la simulación
  Time stop = Seconds (FIN);
  // Se establecen los valores por defecto de las variables
  double tasainicial = MEDIA;

  // Añadimos dos nuevos parametros de entrada
  double paso = 1;
  double valores = 5;

  CommandLine cmd;
  cmd.AddValue ("duracion", "Duración de la simulación", stop);
  cmd.AddValue ("tasaMedia", "Tasa media del proceso de poisson", tasainicial);
  cmd.AddValue ("saltoTasa", "Salto tasa", paso);
  cmd.AddValue ("numValores", "Número valores", valores);

  cmd.Parse (argc, argv);

  Gnuplot2dDataset datos_medias ("medias");
  datos_medias.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  datos_medias.SetErrorBars (Gnuplot2dDataset::Y);
  for (double tasasimulada = tasainicial; tasasimulada <= tasainicial * valores; tasasimulada = tasasimulada + paso)
  {
    // Se crea un objeto de tipo Average, donde los elementos del mismo serán de tipo double
    // Permitirá guardar los valores de interés obtenidos de la simulación
    Average<double> acum;
    NS_LOG_INFO ("Comienza un bucle para calcular la varianza y error de la VA tiempo entre llegadas en un conjunto de experimentos con la misma tasa de llegadas= " << tasasimulada);
    for (int sim = 0; sim < MUESTRAS; sim++)
      {
        double tmp;
        // Definimos el modelo a simular
        Llegadas modelo (tasasimulada);
        Simulator::Stop (stop);
        // Lanzamos la simulación.
        Simulator::Run ();
        acum.Update (tmp = modelo.Media ());
        // Finalizamos la simulación
        Simulator::Destroy ();
        NS_LOG_DEBUG ("GESTIÓN DE ESTADÍSTICAS: Media muestral en el experimento número: " << sim << " = " << modelo.Media()
                      << " cuando se ha usado la media poblacional: " << 1/tasasimulada << " y se ha simulado un tiempo de: " << stop);
      }
    double error = TSTUDENT * sqrt (acum.Var () / acum.Count ());
    datos_medias.Add (tasasimulada,acum.Mean (), error);
    NS_LOG_INFO ("GESTIÓN DE ESTADÍSTICAS: añado un punto a la curva x= " << tasasimulada << " y=: " << acum.Mean() << " + error= " << error);
  }

  // Declaro una Gráfica y la configuro
  Gnuplot GraficaPoisson;
  GraficaPoisson.SetLegend ("Tasa", "Media tiempo entre llegadas");
  GraficaPoisson.SetTitle ("Valor medio del tiempo entre llegadas para distintas tasas");
  // Añado la curva a la gráfica
  GraficaPoisson.AddDataset (datos_medias);
  // Para terminar genero el fichero
  std::ofstream fichero_poisson ("practica01.plt");
  GraficaPoisson.GenerateOutput (fichero_poisson);
  fichero_poisson << "pause -1" << std::endl;
  fichero_poisson.close ();
}
