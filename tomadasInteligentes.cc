// Copyright [2016] <Dúnia Marchiori(14200724) e Vinicius Steffani Schweitzer(14200768)>

#include <gpio.h>
#include <nic.h>
#include <clock.h>
#include <utility/hash.h>

#define INTERVALO_ENVIO_MENSAGENS 5 /*!< Intervalo(em minutos) em que são feitos envio e recebimento de mensagens entre as tomadas. */
#define NUMERO_ENTRADAS_HISTORICO 28 /*!< Quantidade de entradas no histórico. Cada entrada corresponde ao consumo a cada 6 horas. */

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

//----------------------------------------------------------------------------
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
//!  Classe Calendario
/*!
	Classe que guarda quantos dias tem em cada mês do ano.
*/
class Calendario {
	private:
		int diasNoMes[12]; /*!< Vetor que guarda quantos dias tem em cada mês.*/
			
		/*!
			Método que inicializa o vetor com a quantidade de dias em cada mẽs.
		*/
		void inicializarMeses() {
			// 0: Janeiro, 1: Fevereiro, ..., 11: Dezembro
			diasNoMes[0] = 31; 
			diasNoMes[1] = 28;
			diasNoMes[2] = 31;
			diasNoMes[3] = 30;
			diasNoMes[4] = 31;
			diasNoMes[5] = 30;
			diasNoMes[6] = 31;
			diasNoMes[7] = 31;
			diasNoMes[8] = 30;
			diasNoMes[9] = 31;
			diasNoMes[10] = 30;
			diasNoMes[11] =	31;		
		}
	
	public:
		/*!
			Método construtor da classe
		*/
		Calendario() {
			inicializarMeses();
		}
		
		/*!
			Método que retorna quantos dias tem em determinado mês.
			\param mes é o mês que se deseja retornar a quantidade de dias
			\param ano é um inteiro que representa o ano.
			\return Valor inteiro que representa quantos dias tem no mês.
		*/
		int getDiasNoMes(int mes, int ano) {
			if (ano % 4 == 0 && mes == 2) { //se é ano bissexto e o mês é fevereiro
				return 29;
			} else {
				return diasNoMes[mes-1];
			}
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
			/*if (!ligada) { //se está desligada
				
			}*/
			//random consumo
			consumo = 0;
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
		static float preverConsumoProprio(float historico[NUMERO_ENTRADAS_HISTORICO]) { //28 entradas corresponde a 7 dias(uma entrada a cada 6 horas)
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
			\param quartosDeDia é a quantidade de quartos de dia(6 horas) que faltam para o mês acabar.
			\return Valor previsto para o consumo total das tomadas.
		*/
		static float preverConsumoTotal(Simple_Hash<Dados, sizeof(Dados), int>* h, int quartosDeDia) {
			/*Cada previsão é para as próximas 6 horas. Assim, esse valor é multiplicado por quantas mais 6 horas faltam para acabar o mês 
				para sabermos se o consumo está dentro do limite.*/

			float total = 0, consumo;
			for(auto iter = h->begin(); iter != h->end(); iter++) {
				//se iter não é vazio: begin() retorna um objeto vazio no inicio por algum motivo
				if (iter != 0) {
					consumo = iter->object()->consumoPrevisto;
					total += consumo * quartosDeDia;
				}
			}
			return total;
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
		float *historico; /*!< Vetor que guarda o consumo da tomada a cada 6 horas.*/
		//int quantidadeEntradasHistorico; /*!< Variável que indica a quantidade de entradas no histórico já ocupadas.*/
		unsigned int mes; /*!< Variável que indica o mês atual.*/
		Calendario* c; /*!< Objeto que possui informações sobre os dias no mês.*/
		int quantidade6Horas; /*!< Variável que indica a quantidade de quartos de dia(6 horas) que faltam para o fim do mês.*/
		Clock* relogio; /*!< Objeto que possui informações como data e hora.*/
		Mensageiro* mensageiro;	/*!< Objeto que provê a comunicação da placa com as outras.*/
		Simple_Hash<Dados, sizeof(Dados), int>* hash; /*!< Hash que guarda informações recebidas sobre as outras tomadas indexadas pelo endereço da tomada.*/
	
		/*!
			Método que .
		*/
		void sincronizar() {
			bool fim = false;
			Dados *dadosRecebidos, dadosEnviar;
			List_Elements::Singly_Linked_Ordered<Dados, int>* elemento; //elemento da hash
			unsigned int mesAtual = relogio->date().month();
			unsigned int minutosIniciais = relogio->date().minute();
			
			if (mesAtual != mes) {
				consumoMensal = 0; //necessário?
				mes = mesAtual;
				calculaQuantidadeQuartosDeDia();
			}
			
			//adiciona consumo no fim da fila (se está ligada)
			//atualizaHistorico(tomada.getConsumo());
			fazerPrevisaoConsumoProprio();
			dadosEnviar.remetente = tomada->getEndereco();
			dadosEnviar.ligada = tomada->estaLigada();
			dadosEnviar.consumoPrevisto = consumoProprioPrevisto;
			dadosEnviar.prioridade = prioridadeAtual();
			
			unsigned int minutoAtual = relogio->date().minute();
			unsigned int ultimoSend = minutoAtual;
			while (minutoAtual < minutosIniciais + INTERVALO_ENVIO_MENSAGENS) {
				if (minutoAtual - ultimoSend >= 1) { //envia a cada um minuto
					enviarMensagemBroadcast(dadosEnviar);
				} else {
					dadosRecebidos = &receberMensagem(); // pra tirar o & o receberMensagem precisa retornar um ponteiro
					elemento = new List_Elements::Singly_Linked_Ordered<Dados, int>(dadosRecebidos,dadosRecebidos->remetente); //hash é indexada pelo endereço da tomada
					atualizaHash(elemento);
				}
				minutoAtual = relogio->date().minute();		
			}

			fazerPrevisaoConsumoTotal(); // considera todas as tomadas, mesmo as desligadas			
			if (consumoTotalPrevisto > maximoConsumoMensal) { // se a previsão está acima do consumo máximo
				if (deveDesligar()) {
					tomada->desligar();
				}
				while (relogio->date().minute() < minutosIniciais + INTERVALO_ENVIO_MENSAGENS + 1); // necessário?
			} else { // se a previsão está dentro do consumo máximo
				//liga todas
				//podeLigar();	-> tomada->ligar();			
			}
			quantidade6Horas--;
		}
	
		/*!
			Método que atualiza o vetor de histórico de consumo da tomada com o novo consumo. Consumo nulo(tomada desligada) não é inserido.
			\param novo é o consumo atual da tomada que será inserido no histórico.
		*/
		void atualizaHistorico(float novo) {
			if (tomada->estaLigada()) { 
				int i;
				for (i = 0; i < (NUMERO_ENTRADAS_HISTORICO - 2); ++i) {
					historico[i] = historico[i+1];
				}
				//consumo novo é adicionado no fim do vetor para manter coerência com a lógica da previsão de consumo
				historico[NUMERO_ENTRADAS_HISTORICO - 1] = novo;
			}			
		}
	
		/*!
			Método que atualiza a entrada da hash correspondente ao elemente passado por parâmetro. Se ele não existir, é adicionado.
			\param e é o elemento a ser atualizado ou adicionado na hash.
		*/
		void atualizaHash(List_Elements::Singly_Linked_Ordered<Dados, int>* e) {
			Dados* d =  hash->search_key(e->object()->remetente)->object();
			if (d != 0) {
					d->ligada = e->object()->ligada;
					d->consumoPrevisto = e->object()->consumoPrevisto;
					d->prioridade = e->object()->prioridade;
			} else if(e->object()->remetente != (Address)-1) { 
				//elemento não está na hash e não é um elemento "vazio"(remetente é igual a -1 quando não há mensagem recebida)
				hash->insert(e);	
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
			calculaQuantidadeQuartosDeDia();
			consumoMensal = 0;
			mensageiro = new Mensageiro();
			c = new Calendario();
			hash = new Simple_Hash<Dados, sizeof(Dados), int>();
			historico = new float[NUMERO_ENTRADAS_HISTORICO];
			//inicializar historico com 0
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
			Método que atualiza o valor da previsão do consumo da tomada até o fim do mês. O valor é armazenado na variável global consumoProprioPrevisto. 
			\sa Previsor
		*/
		void fazerPrevisaoConsumoProprio() {
			consumoProprioPrevisto = Previsor::preverConsumoProprio(historico);
		}

		/*!
			Método que atualiza o valor da previsão do consumo total das to até o fim do mês. O valor é armazenado na variável global consumoTotalPrevisto. 
		*/
		void fazerPrevisaoConsumoTotal() {
			consumoTotalPrevisto = Previsor::preverConsumoTotal(hash, quantidade6Horas);
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
			\return Valor da prioridade da tomada no período atual do dia.
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
		bool deveDesligar(); //verificar se pode dimmerizar
	
		/*!
			Método que calcula quantos dias restam para o mês acabar.
			\return Valor inteiro que indica quantos dias faltam para o fim do mês.
		*/
		int diasRestantes() {
			unsigned int ano = relogio->date().year();
			unsigned int diaAtual = relogio->date().day();
			int diasNoMes = c->getDiasNoMes(mes, ano);
			return (diasNoMes - diaAtual);		
		}
	
		/*!
			Método que calcula quantos 1/4 de dia faltam para o fim do mês. O valor é armazenado na variável global quantidade6Horas.
		*/
		void calculaQuantidadeQuartosDeDia() {
			quantidade6Horas = diasRestantes() * 4;
			int hora = relogio->date().hour();		
			// ajusta da quantidade para quando a tomada é criada depois do primeiro 1/4 do dia.
			int quartoDoDia = (int) hora / 6;
			quantidade6Horas = quantidade6Horas - quartoDoDia;		
		}
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
