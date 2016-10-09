// Copyright [2016] <Dúnia Marchiori(14200724) e Vinicius Steffani Schweitzer(14200768)>

#include <gpio.h>
#include <nic.h>

using namespace EPOS;

typedef NIC::Address Address;
typedef NIC::Protocol Protocol;

//!  Struct Dados
/*!
	Agrupamento dos dados que são transmitidos entre as placas.
*/
struct Dados {
	Address remetente; /*!< Endereço da tomada rementente da mensagem */
    float consumoPrevisto; /*!< Corresponde ao consumo previsto da tomada */
    int prioridade; /*!< Corresponde à prioridade da tomada */
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
			Método que transmite uma mensagem em Broadcast.
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
			const Protocol prot = 1;
			nic->send(destino, prot, (void*) &msg, sizeof msg); // Protocol é 1, o que isso significa? Não sei, tava no send_recieve.cc
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
			
			if (!hasMsg) { // Se não chegou mensagem nenhuma
				msg.remetente = -1;
				msg.consumoPrevisto = -1;
				msg.prioridade = -1;
			}
			
			return msg; // Quero mesmo retornar só a msg?
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
		GPIO *led; /*!< Variável que representa o LED.*/

	public:
		/*!
			Método construtor da classe
		*/
		Led() {
			led = new GPIO('C',3, GPIO::OUTPUT);
		}

		/*!
			Método que acende o LED do EPOSMoteIII.
		*/
    	void acenderLED() {
    		led->set(true);
    	}

    	/*!
			Método que desliga o LED do EPOSMoteIII.
    	*/
    	void desligarLED() {
    		led->set(false);
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
		TomadaComDimmer();

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
		int prioridade; /*!< Variável que indica a prioridade da tomada.*/
		float consumo; /*!< Variável que indica o consumo da tomada.*/
		int endereco; /*!< Variável que indica o endereço da tomada.*/

	public:
		/*!
			Método construtor da classe
		*/
		TomadaInteligente();

		/*!
			Método que altera a prioridade da tomada para o valor passado por parâmetro.
			\param p é a prioridade da tomada.
		*/
		void setPrioridade(int p) {
			prioridade = p;
		}

		/*!
			Método que retorna o valor da prioridade da tomada.
			\return Valor inteiro que indica a prioridade da tomada.
		*/
		int getPrioridade() {
			return prioridade;
		}

		/*!
			Método que retorna o consumo atual da tomada.
			\return Valor float que indica o consumo atual da tomada. Caso esteja desligada, o valor retornado é 0.
		*/
		float getConsumo() {
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
		TomadaMulti();
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
		Previsor();

		/*!
			Método que estima o consumo da tomada até o fim do mês.
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
			Método que estima o consumo de todas as tomadas juntas.
			\return Valor previsto para o consumo total das tomadas.
		*/
		static float preverConsumoTotal();

};


//----------------------------------------------------------------------------
//!  Classe Gerente
/*!
	Classe que faz o controle das tomadas inteligentes com EPOSMoteIII.
*/
class Gerente {
	private:
		Tomada tomada; /*!< Variável que indica a tomada que o gerente controla.*/
		float maximoConsumoMensal; /*!< Variável que indica o máximo de consumo que as tomadas podem ter mensalmente.*/
		float consumoProprioPrevisto; /*!< Variável que indica o consumo previsto da tomada no mês.*/
		float consumoTotalPrevisto; /*!< Variável que indica o consumo total previsto no mês.*/
		float historico[31];
		
		//Mensageiro ?
		//retrição de horários

	public:
		/*!
			Método construtor da classe
		*/
		Gerente();

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
		void enviarMensagem();

		/*!
			Método que recebe mensagem das outras tomadas.
		*/
		void receberMensagem();

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
};


//----------------------------------------------------------------------------
//!  Método Main
/*!
	Método inicial do programa.
*/
int main() {
	Tomada *t = new Tomada();
};
