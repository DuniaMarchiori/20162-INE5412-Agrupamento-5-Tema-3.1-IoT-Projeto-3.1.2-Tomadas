// Copyright [2016] <Dúnia Marchiori(14200724) e Vinicius Steffani Schweitzer(14200768)>

#include <gpio.h>
#include <nic.h>
#include <clock.h>

#define INTERVALO_ENVIO_MENSAGENS 5 /*!< Intervalo(em minutos) em que são feitos envio e recebimento de mensagens entre as tomadas. */

using namespace EPOS;

typedef NIC::Address Address;
typedef NIC::Protocol Protocol;

//!  Struct Prioridades
/*!
	Struct contendo as prioridades da tomada ao longo do dia.
*/
struct Prioridades {
    int madrugada; /*!< Corresponde à prioridade da tomada no horario das 00:00 (incluso) às 06:00 (não incluso). */
    int dia; /*!< Corresponde à prioridade da tomada no horario das 06:00 (incluso) às 12:00 (não incluso). */
    int tarde; /*!< Corresponde à prioridade da tomada no horario das 12:00 (incluso) às 18:00 (não incluso). */
    int noite; /*!< Corresponde à prioridade da tomada no horario das 18:00 (incluso) às 00:00 (não incluso). */
};

//!  Struct Dados
/*!
	Agrupamento dos dados que serão transmitidos e recebidos pela placa.
*/
struct Dados {
    Address remetente; /*!< Endereço da tomada remetente da mensagem. */
    bool ligada; /*!< Indica se a tomada remetente está ligada. */
    float consumoPrevisto; /*!< Corresponde ao consumo previsto da tomada. */
    int prioridade; /*!< Corresponde à prioridade da tomada. */
};

//!  Classe Mensageiro
/*!
	Classe encarregada de enviar e receber mensagens.
*/
class Mensageiro {
	private:
		NIC * nic; /*!< Variável que representa o NIC.*/

	public:
		/*!
			Método construtor da classe
		*/
		Mensageiro() {
			nic = new NIC();
		}
		
		/*!
			Método que transmite uma mensagem em broadcast.
			\param msg é a mensagem que será enviada.
		*/
		void enviarBroadcast(Dados msg) {
			enviarMensagem(nic->broadcast(), msg);
		}
		
		/*!
			Método que transmite uma mensagem para um destinatário.
			\param destino é o endereço do dispositivo destinatário.
			\param msg é a mensagem que será enviada.
		*/
		void enviarMensagem(const Address destino, const Dados msg) {
			const Protocol prot = 1; // Porque o protocol é 1?
			nic->send(destino, prot, &msg, sizeof msg);
		}
		
		/*!
			Método que recebe uma mensagem.
			\return Retorna a mensagem recebida.
		*/
		Dados receberMensagem() {
			Address remetente;
			Protocol prot;
			Dados msg;
			bool hasMsg = nic->receive(&remetente, &prot, &msg, sizeof msg);
			
			if (!hasMsg) { // Se não foi recebida nenhuma mensagem
				msg.remetente = -1;
				msg.consumoPrevisto = -1;
				msg.prioridade = -1;
			}
			
			return msg; // Retornar só a msg?
		}
		
		/*!
			Método que retorna o endereço NIC do dispositivo.
			\return Valor do tipo Address que representa o endereço do dispositivo.
		*/
		Address obterEnderecoNIC() {
			return nic->address();
		}
};

//----------------------------------------------------------------------------
//!  Classe Led
/*!
	Classe que faz o controle do LED do EPOSMoteIII.
*/
class Led {
	private:
		//GPIO *led; /*!< Variável que representa o LED.*/

	public:
		/*!
			Método construtor da classe
		*/
		Led() {
			//led = new GPIO('C',3, GPIO::OUTPUT);
		}

		/*!
			Método que acende o LED do EPOSMoteIII.
		*/
    	void acenderLED() {
    		//led->set(true);
    	}

    	/*!
			Método que desliga o LED do EPOSMoteIII.
    	*/
    	void desligarLED() {
    		//led->set(false);
    	}

};


//----------------------------------------------------------------------------
//!  Classe Tomada
/*!
	Classe que representa uma tomada.
*/
class Tomada {
	private:
		bool ligada; /*!< Variável booleana que indica se a tomada está ligada.*/
		Led *led; /*!< Variável que representa o LED.*/

	public:
		/*!
			Método construtor da classe
		*/
		Tomada() {
			ligada = false;
			led  = new Led();
		}

		/*!
			Método que verifica se a tomada está ligada ou não.
			\return Valor booleano que indica se a tomada está ligada.
		*/
		bool estaLigada() {
			return ligada;
		}

		/*!
			Método que liga a tomada.
		*/
		void ligar() {
			ligada = true;
			led->acenderLED();
		}

		/*!
			Método que desliga a tomada.
		*/
		void desligar() {
			ligada = false;
			led->desligarLED();
		}
};


//----------------------------------------------------------------------------
//!  Classe TomadaComDimmer
/*!
	Classe que representa uma tomada com dimmer.
*/
class TomadaComDimmer: public Tomada {
	private:
		float dimPorcentagem; /*!< Váriavel float que indica a porcentagem de dimmerização da tomada.*/

	public:
		/*!
			Método construtor da classe
		*/
		//TomadaComDimmer();

		/*!
			Método que retorna a a porcentagem de dimmerização da tomada.
			\return Valor float que indica a porcentagem de dimmerização da tomada.
		*/
		float getPorcentagem() {
			return dimPorcentagem;
		}

		/*!
			Método que dimmeriza a tomada de acordo com o porcentagem passada.
			\param p é a porcentagem de dimmerização.
		*/
		void dimmerizar(float p);
};


//----------------------------------------------------------------------------
//!  Classe TomadaInteligente
/*!
	Classe que representa uma tomada controlada pelo EPOSMoteIII.
*/
class TomadaInteligente: public Tomada {
	private:
		Prioridades prioridades; /*!< Variável que contém as prioridade da tomada ao longo do dia.*/
		float consumo; /*!< Variável que indica o consumo da tomada.*/
		int endereco; /*!< Variável que indica o endereço da tomada.*/

	public:
		/*!
			Método construtor da classe
		*/
		//TomadaInteligente();

		/*!
			Método que altera todas as prioridades da tomada para o valor passado por parâmetro.
			\param p é o novo valor das prioridades da tomada.
		*/
		void setPrioridades(int p) {
			prioridades.madrugada = p;
			prioridades.dia = p;
			prioridades.tarde = p;
			prioridades.noite = p;
		}
		
		/*!
			Método que altera a prioridade no periodo das 00:00 às 06:00.
			\param p é o novo valor da prioridade nesse horario.
		*/
		void setPrioridadeMadrugada(int p) {
			prioridades.madrugada = p;
		}
		
		/*!
			Método que altera a prioridade no periodo das 06:00 às 12:00.
			\param p é o novo valor da prioridade nesse horario.
		*/
		void setPrioridadeDia(int p) {
			prioridades.dia = p;
		}
		
		/*!
			Método que altera a prioridade no periodo das 12:00 às 18:00.
			\param p é o novo valor da prioridade nesse horario.
		*/
		void setPrioridadeTarde(int p) {
			prioridades.tarde = p;
		}
		
		/*!
			Método que altera a prioridade no periodo das 18:00 às 00:00.
			\param p é o novo valor da prioridade nesse horario.
		*/
		void setPrioridadeNoite(int p) {
			prioridades.noite = p;
		}

		/*!
			Método que retorna as prioridades da tomada.
			\return Struct que contém as quatro possiveis prioridades da tomada.
		*/
		Prioridades getPrioridades() {
			return prioridades;
		}

		/*!
			Método que retorna o consumo atual da tomada.
			\return Valor float que indica o consumo atual da tomada. Caso esteja desligada, o valor retornado é 0.
		*/
		float getConsumo() {
			if (!ligada) { //se está desligada
				consumo = 0;
			}
			//random consumo
			return consumo;
		}

		/*!
			Método que retona o endereço da tomada.
			\return Valor inteiro que indica o endereço da tomada.
		*/
		int getEndereco() {
			return endereco;
		}
};


//----------------------------------------------------------------------------
//!  Classe TomadaMulti
/*!
	Classe que representa uma tomada com dimmer controlada pelo EPOSMoteIII.
*/
class TomadaMulti: public TomadaComDimmer, public TomadaInteligente {
	public:
		/*!
			Método construtor da classe
		*/
		//TomadaMulti();
};


//----------------------------------------------------------------------------
//!  Classe Previsor
/*!
	Classe que faz a previsão de consumo de uma determinada tomada.
*/
class Previsor { 
	public:
		/*!
			Método construtor da classe
		*/
		//Previsor();

		/*!
			Método que estima o consumo da tomada para as pŕoximas 6 horas.
			\return Valor previsto para o consumo da tomada.
		*/
		static float preverConsumoProprio(float historico[28]) { //28 entradas corresponde a 7 dias(uma entrada a cada 6 horas)
     		/*float previsao = 0.03* historico[0]+ 0.04* historico[1] + 0.06* historico[2] + 0.07* historico[3] 
     						+ 0.09* historico[4] + 0.1* historico[5] + 0.11* historico[6] + 0.14* historico[7] 
     						+ 0.16* historico[8] + 0.20* historico[9];*/
     		/*float previsao = 0.* historico[0]   + 0.* historico[1]  + 0.* historico[2]  + 0.* historico[3] 
            				+ 0.* historico[4]  + 0.* historico[5]  + 0.* historico[6]  + 0.* historico[7] 
                        	+ 0.* historico[8]  + 0.* historico[9]  + 0.* historico[10] + 0.* historico[11] 
                        	+ 0.* historico[10] + 0.* historico[13] + 0.* historico[14] + 0.* historico[15] 
                        	+ 0.* historico[16] + 0.* historico[17] + 0.* historico[18] + 0.* historico[19] 
                        	+ 0.* historico[20] + 0.* historico[21] + 0.* historico[22] + 0.* historico[23] 
                        	+ 0.* historico[24] + 0.* historico[25] + 0.* historico[26] + 0.* historico[27];*/

            float previsao = 0.015* historico[0]    + 0.017* historico[1]   + 0.018* historico[2]   + 0.02* historico[3] //7%
                        	+ 0.02* historico[4]    + 0.025* historico[5]   + 0.027* historico[6]   + 0.028* historico[7] //10%
                        	+ 0.028* historico[8]   + 0.029* historico[9]   + 0.03* historico[10]   + 0.033* historico[11] //12%
                        	+ 0.033* historico[10]  + 0.034* historico[13]  + 0.035* historico[14]  + 0.038* historico[15] //14%
                        	+ 0.045* historico[16]  + 0.045* historico[17]  + 0.045* historico[18]  + 0.045* historico[19] //18%
                        	+ 0.0475* historico[20] + 0.0475* historico[21] + 0.0475* historico[22] + 0.0475* historico[23] //19%
                        	+ 0.05* historico[24]   + 0.05* historico[25]   + 0.05* historico[26]   + 0.05* historico[27]; //20%
        	return previsao;
    	}

		/*!
			Método que estima o consumo de todas as tomadas juntas até o fim do mês.
			\return Valor previsto para o consumo total das tomadas.
		*/
		static float preverConsumoTotal() {
			/*Cada previsão é para as próximas 6 horas. Assim, esse valor é multiplicado por quantas mais 6 horas faltam para acabar o mês 
				para sabermos se o consumo está dentro do limite.*/
			int quartosDeDia = 1;
			// para cada previsão de tomada que está ligada: previsão * quartos de dia
			//soma tudo e retorna
		}

};


//----------------------------------------------------------------------------
//!  Classe Gerente
/*!
	Classe que faz o controle das tomadas inteligentes com EPOSMoteIII.
*/
class Gerente {
	private:
		TomadaInteligente* tomada; /*!< Variável que indica a tomada que o gerente controla.*/
		float maximoConsumoMensal; /*!< Variável que indica o máximo de consumo que as tomadas podem ter mensalmente.*/
		float consumoMensal; /*!< Variável que indica o consumo mensal das tomadas até o momento.*/
		float consumoProprioPrevisto; /*!< Variável que indica o consumo previsto da tomada no mês.*/
		float consumoTotalPrevisto; /*!< Variável que indica o consumo total previsto no mês.*/
		float historico[28]; /*!< Vetor que guarda o consumo da tomada a cada 6 horas.*/
		unsigned int mes; /*!< Variável que indica o mês atual.*/
		Clock* relogio; /*!< Objeto que possui informações como data e hora.*/
		Mensageiro* mensageiro;	/*!< Objeto que provê a comunicação da placa com as outras.*/
	
		/*!
			Método que .
		*/
		void sincronizar() {
			bool fim = false;
			Dados dadosRecebidos, dadosEnviar;
			unsigned int mesAtual = relogio->date().month();
			unsigned int minutosIniciais = relogio->date().minute();
			
			if (mesAtual != mes) { //necessário?
				consumoMensal = 0;
				mes = mesAtual;
			}
			
			while(!fim) {
				//adiciona consumo no fim da fila -> se está ligada
				fazerPrevisaoConsumoProprio();
				dadosEnviar.remetente = tomada->getEndereco();
				dadosEnviar.ligada = tomada->estaLigada();
				dadosEnviar.consumoPrevisto = consumoProprioPrevisto;
				dadosEnviar.prioridade = prioridadeAtual();
				while (relogio->date().minute() < minutosIniciais + INTERVALO_ENVIO_MENSAGENS) {
					enviarMensagemBroadcast(dadosEnviar);
					dadosRecebidos = receberMensagem();
					//atualiza registro das outras tomadas
				}
				fazerPrevisaoConsumoTotal(); // passar tabela das outras tomadas
				if (consumoTotalPrevisto > maximoConsumoMensal) { // se a previsão está acima do consumo máximo
					if (deveDesligar()) {
						tomada->desligar();
					}
					while (relogio->date().minute() < minutosIniciais + INTERVALO_ENVIO_MENSAGENS + 1);
				} else { // se a previsão está dentro do consumo máximo
					//vê se a tomada desligada pode ligar, baseada na previsão do consumo dela e do consumo total
					//podeLigar();
					fim = true;				
				}
			}
		}

	public:
		/*!
			Método construtor da classe
		*/
		Gerente(TomadaInteligente* t) {
			relogio = new Clock();
			tomada = t;
			mes = relogio->date().month();
			consumoMensal = 0;
			mensageiro = new Mensageiro();
		}

		/*!
			Método que altera o valor do consumo mensal máximo para o valor passado por parâmetro.
			\param consumo é o consumo máximo mensal.
		*/
		void setConsumoMensalMaximo(float consumo) {
			maximoConsumoMensal = consumo;
		}

		/*!
			Método que envia mensagem para as outras tomadas.
		*/
		void enviarMensagemBroadcast(Dados d) {
			mensageiro->enviarBroadcast(d);
		}

		/*!
			Método que recebe mensagem das outras tomadas.
		*/
		Dados receberMensagem() {
			return mensageiro->receberMensagem();
		}

		/*!
			Método que atualiza o valor da previsão do consumo da tomada até o fim do mês.
			\sa Previsor
		*/
		void fazerPrevisaoConsumoProprio() {
			consumoProprioPrevisto = Previsor::preverConsumoProprio(historico);
		}

		/*!
			Método que atualiza o valor da previsão do consumo total das to até o fim do mês.
		*/
		void fazerPrevisaoConsumoTotal() {
			consumoTotalPrevisto = Previsor::preverConsumoTotal();
		}

		/*!
			Método que realiza a sincronização entre as tomadas a cada 6 horas.
			\sa sincronizar()
		*/
		void iniciar() {
			//while true {
			sincronizar();
			//while não é hora de acordar}
		}
		
		/*!
			Método que baseado no horário atual descobre a prioridade certa.
			\return valor da prioridade da tomada no período atual do dia.
		*/
		int prioridadeAtual() {
			Prioridades prioridades = tomada->getPrioridades();
			unsigned int hour = relogio->date().hour();
			int quarterOfDay = (int) hour / 6;
			
			switch(quarterOfDay){
				case 0:
					return prioridades.madrugada;
				case 1:
					return prioridades.dia;
				case 2:
					return prioridades.tarde;
				case 3:
					return prioridades.noite;
			}
			
		}

		/*!
			Método que verifica se a tomada deve desligar para manter o consumo mensal dentro do consumo máximo.
			\return valor booleano que indica se a tomada deve desligar.
		*/
		bool deveDesligar();
};


//----------------------------------------------------------------------------
//!  Método Main
/*!
	Método inicial do programa.
*/
int main() {
	TomadaInteligente* t = new TomadaInteligente(); 
	//t.setPrioridades
	Gerente* g = new Gerente(t);
	//g.iniciar();
};
