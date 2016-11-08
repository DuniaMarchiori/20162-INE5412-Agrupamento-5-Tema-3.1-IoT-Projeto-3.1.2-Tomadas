// Copyright [2016] <Dúnia Marchiori(14200724) e Vinicius Steffani Schweitzer(14200768)>

#include <gpio.h>
#include <nic.h>
#include <utility/hash.h>
#include <utility/random.h>
#include <alarm.h>
#include <chronometer.h>
#include <usb.h>
#include <utility/math.h>
#include <utility/string.h>


#define INTERVALO_ENVIO_MENSAGENS 5 /*!< Intervalo(em minutos) em que são feitos envio e recebimento de mensagens entre as tomadas. */
#define NUMERO_ENTRADAS_HISTORICO 28 /*!< Quantidade de entradas no histórico. Cada entrada corresponde ao consumo a cada 6 horas. */
#define NUMERO_CHAR_CONFIG 40 /*!< Quantidade máxima de caracteres por mensagem. */

using namespace EPOS;

OStream cout;

typedef NIC::Address Address;
typedef NIC::Protocol Protocol;

//!  Struct Prioridades
/*!
	Struct contendo as prioridades da tomada ao longo do dia.
*/
struct Prioridades {
    int madrugada; /*!< Corresponde à prioridade da tomada no horário das 00:00 (incluso) às 06:00 (não incluso). */
    int manha; /*!< Corresponde à prioridade da tomada no horário das 06:00 (incluso) às 12:00 (não incluso). */
    int tarde; /*!< Corresponde à prioridade da tomada no horário das 12:00 (incluso) às 18:00 (não incluso). */
    int noite; /*!< Corresponde à prioridade da tomada no horário das 18:00 (incluso) às 00:00 (não incluso). */
};

//!  Struct Dados
/*!
	Agrupamento dos dados que serão transmitidos e recebidos pelo EPOSMoteIII.
*/
struct Dados {
    Address remetente; /*!< Endereço da tomada remetente da mensagem. */
    //bool ligada; /*!< Indica se a tomada remetente está ligada. */
    float consumoPrevisto; /*!< Corresponde ao consumo previsto da tomada até o fim do mês. */
	float ultimoConsumo; /*!< Corresponde ao valor do consumo da tomada nas últimas 6 horas. */
    int prioridade; /*!< Corresponde à prioridade da tomada no período de envio da mensagem. */
    char configuracao[NUMERO_CHAR_CONFIG]; /*!< É uma possível configuração que precise ser feita pela tomada. */
	bool podeDesligar; /*!< Indica se a tomada pode ser desligada no período de envio da mensagem. */
};

//!  Struct Data
/*!
	Struct contendo valores de uma data.
*/
struct Data {
	long long dia; /*!< Variável que representa o dia atual.*/
	long long mes; /*!< Variável que representa o mês atual.*/
	long long ano; /*!< Variável que representa o ano atual.*/
	long long hora; /*!< Variável que representa a hora atual.*/
	long long minuto; /*!< Variável que representa os minutos atuais.*/
	long long segundo; /*!< Variável que representa os segundos atuais.*/
	long long microssegundos; /*!< Variável que representa os microssegundos atuais.*/
};

typedef List_Elements::Singly_Linked_Ordered<Dados, Address> Hash_Element;
typedef Simple_Hash<Dados, sizeof(Dados), Address> Tabela;

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
			Método construtor da classe.
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
			const Protocol prot = NIC::PTP;
			nic->send(destino, prot, &msg, sizeof msg);
		}

		/*!
			Método que recebe uma mensagem.
			\return Retorna a mensagem recebida.
		*/
		Dados* receberMensagem() {
			Address* remetente;
			Protocol* prot;
			Dados* msg = new Dados();

			bool hasMsg = nic->receive(remetente, prot, msg, sizeof *msg);

			if (!hasMsg) { // Se não foi recebida nenhuma mensagem
				//msg->remetente = -1;
				//msg->ligada = false;
				msg->consumoPrevisto = -1;
				msg->ultimoConsumo = -1;
				msg->prioridade = -1;
				msg->configuracao[0] = '\0';
				msg->podeDesligar = -1;
			} else {
				cout << "   Mensagem Recebida de " << msg->remetente << endl;
			}

			return msg;
		}

		/*!
			Método que retorna o endereço NIC do dispositivo.
			\return Valor do tipo Address que representa o endereço do dispositivo.
		*/
		const Address obterEnderecoNIC() {
			return nic->address();
		}

		/*!
			Método que retorna um endereço, convertendo seus valores hexadecimais para decimais.
			\param endereco string com o endereço em hexadecimal.
			\param retorno ponteiro que recebera a string convertida.
		*/
		void converterEndereco(char* endereco, char* retorno) {
			char endA[3];
			char endB[3];

			int numA = 0;
			int numB = 0;
			int num = 0;
			char c;
			for (int i = 0; i < 5; i++) {
				if (i == 2) {
					i++;
				}
				c = endereco[i];
				switch (c) {
					case 'a':
						num = 10;
						break;
					case 'b':
						num = 11;
						break;
					case 'c':
						num = 12;
						break;
					case 'd':
						num = 13;
						break;
					case 'e':
						num = 14;
						break;
					case 'f':
						num = 15;
						break;
					default:
						num = c - '0';
						break;
				}
				if (i < 2) {
					numA += num*pow(16, (1-i));
				} else {
					numB += num*pow(16, (4-i));
				}
			}

			for (int i = 0; i < 3; i++) {
				endA[2-i] = (numA % 10) + '0';
				numA /= 10;
				endB[2-i] = (numB % 10) + '0';
				numB /= 10;
			}

			for (int i = 0; i < 6; i++) {
				if (i < 3) {
					retorno[i] = endA[i];
				} else {
					retorno[i+1] = endB[i-3];
				}
			}
			retorno[3] = ':';
		}
};

//----------------------------------------------------------------------------
//!  Classe Relogio
/*!
	Classe que lida com as informações de datas e horários.
*/
class Relogio {
	private:
		Data data; /*!< É uma struct Data para o controle da data atual.*/
		int diasNoMes[12]; /*!< Vetor que guarda quantos dias tem em cada mês.*/
		Chronometer* cronometro; /*!< Objeto da classe Chronometer que representa um cronômetro.*/

		/*!
			Método que inicializa o vetor com a quantidade de dias em cada mẽs.
		*/
		void inicializarMeses() {
			diasNoMes[0]  = 31; // Janeiro;
			diasNoMes[1]  = 28; // Fevereiro;
			diasNoMes[2]  = 31; // Março;
			diasNoMes[3]  = 30; // Abril;
			diasNoMes[4]  = 31; // Maio;
			diasNoMes[5]  = 30; // Junho;
			diasNoMes[6]  = 31; // Julho;
			diasNoMes[7]  = 31; // Agosto;
			diasNoMes[8]  = 30; // Setembro;
			diasNoMes[9]  = 31; // Outubro;
			diasNoMes[10] = 30; // Novembro;
			diasNoMes[11] =	31;	// Dezembro.
		}

	public:
		/*!
			Método construtor da classe.
		*/
		/*Relogio(int a, int m, int d, int h, int min) {
			cronometro = new Chronometer();
			data.ano = a;
			data.mes= m;
			data.dia = d;
			data.hora = h;
			data.minuto = min;
			data.segundo = 0;
			data.microssegundos = 0;
			cronometro->reset();
			cronometro->start();
			inicializarMeses();
		}*/
		Relogio() {
			cronometro = new Chronometer();

			// data default
			data.ano = 2016;
			data.mes= 1;
			data.dia = 1;
			data.hora = 0;
			data.minuto = 0;

			data.segundo = 0;
			data.microssegundos = 0;
			cronometro->reset();
			cronometro->start();
			inicializarMeses();
		}

		/*!
			Método que retorna o dia atual.
			\return um inteiro que representa o dia atual.
		*/
		Data getData() {
			atualizaRelogio();
			return data;
		}

		void setData(Data d) {
			data = d;
		}

		void setAno(int a) {
			data.ano = a;
		}

		void setMes(int m) {
			data.mes= m;
		}

		void setDia(int d) {
			data.dia = d;
		}

		void setHora(int h) {
			data.hora = h;
		}

		void setMinuto(int m) {
			data.minuto = m;
		}

		/*!
			Método que atualiza os dados do relógio, baseado no tempo desde a última requisição.
		*/
		void atualizaRelogio() {
			long long tempoDecorrido = cronometro->read();
			cronometro->reset();
			cronometro->start();
			incrementarMicrossegundo(tempoDecorrido);
		}

		/*!
			Método que incrementa o ano.
			\param anos é a quantidade de anos que serão incrementados no relógio.
		*/
		void incrementarAno(long long anos){
			data.ano+=anos;
		}

		/*!
			Método que incrementa os meses de forma a atualizar todas as unidades de tempo superiores.
			\param meses é a quantidade de meses que serão incrementados no relógio.
		*/
		void incrementarMes(long long meses){
			data.mes+=meses;
			if (data.mes > 12) {
				incrementarAno( data.mes/12 );
				data.mes = data.mes % 12;
			}
		}

		/*!
			Método que incrementa os dias de forma a atualizar todas as unidades de tempo superiores.
			\param dias é a quantidade de dias que serão incrementados no relógio.
		*/
		void incrementarDia(long long dias){
			while (dias > 0) {
				data.dia++;
				if (data.dia > getDiasNoMes(data.mes, data.ano)) {
					incrementarMes(1);
					data.dia = 1;
				}
				dias--;
			}
		}

		/*!
			Método que incrementa as horas de forma a atualizar todas as unidades de tempo superiores.
			\param hrs é a quantidade de horas que serão incrementadas no relógio.
		*/
		void incrementarHora(long long hrs){
			data.hora+=hrs;
			if (data.hora >= 24) {
				incrementarDia( data.hora/24 );
				data.hora = data.hora % 24;
			}
		}

		/*!
			Método que incrementa os minutos de forma a atualizar todas as unidades de tempo superiores.
			\param mins é a quantidade de minutos que serão incrementados no relógio.
		*/
		void incrementarMinuto(long long mins){
			data.minuto+=mins;
			if (data.minuto >= 60) {
				incrementarHora( data.minuto/60 );
				data.minuto = data.minuto % 60;
			}
		}

		/*!
			Método que incrementa os segundos de forma a atualizar todas as unidades de tempo superiores.
			\param secs é a quantidade de segundos que serão incrementados no relógio.
		*/
		void incrementarSegundo(long long secs){
			data.segundo+=secs;
			if (data.segundo >= 60) {
				incrementarMinuto( data.segundo/60 );
				data.segundo = data.segundo % 60;
			}
		}

		/*!
			Método que incrementa os microssegundos de forma a atualizar todas as unidades de tempo superiores.
			\param mcsec é a quantidade de microssegundos que serão incrementados no relógio.
		*/
		void incrementarMicrossegundo(long long mcsec){
			data.microssegundos+=mcsec;
			if (data.microssegundos >= 1000000) {
				incrementarSegundo( data.microssegundos/1000000 );
				data.microssegundos = data.microssegundos % 1000000;
			}
		}

		/*!
			Método que retorna quantos dias tem no mês atual.
 			\param mes é um inteiro que indica o mês atual.
 			\param ano é um inteiro que indica o ano atual.
			\return Valor inteiro que representa quantos dias tem no mês.
		*/
		int getDiasNoMes(int mes, int ano) {
			if (ano % 4 == 0 && mes == 2) { // Se é ano bissexto e o mês é fevereiro.
				return 29;
			} else {
				return diasNoMes[mes-1];
			}
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
	protected:
		bool ligada; /*!< Variável booleana que indica se a tomada está ligada.*/
	private:
		Led *led; /*!< Variável que representa o LED.*/

	public:
		/*!
			Método construtor da classe
		*/
		Tomada() {
			ligada = true;
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
class TomadaComDimmer: virtual public Tomada {
	protected:
		float dimPorcentagem; /*!< Váriavel float que indica a porcentagem de dimmerização da tomada.*/

	public:
		/*!
			Método construtor da classe
		*/
		TomadaComDimmer() {
			dimPorcentagem = 1; // 100%
		}

		/*!
			Método que retorna a porcentagem de dimmerização da tomada.
			\return Valor float que indica a porcentagem de dimmerização da tomada.
		*/
		float getPorcentagem() {
			return dimPorcentagem;
		}
};

//----------------------------------------------------------------------------
//!  Classe TomadaInteligente
/*!
	Classe que representa uma tomada controlada pelo EPOSMoteIII.
*/
class TomadaInteligente: virtual public Tomada {
	protected:
		int tipo; /*!< Variável que indica o tipo da tomada. Tipo 1 indica uma TomadaInteligente*/
		float consumo; /*!< Variável que indica o consumo da tomada.*/
	private:
		Prioridades prioridades; /*!< Variável que contém as prioridade da tomada ao longo do dia.*/
		bool podeDesligar[4];

	public:
		/*!
			Método construtor da classe
		*/
		TomadaInteligente() {
			consumo = 0;
			tipo = 1; // Indica uma TomadaInteligente
			setPrioridades(1); // Prioridade padrão é 1
			setPodeDesligar(true, 0);
			setPodeDesligar(true, 1);
			setPodeDesligar(true, 2);
			setPodeDesligar(true, 3);
		}

		/*!
			Método que devolve o tipo de tomada.
			\return o tipo dessa tomada.
		*/
		int getTipo(){
			return tipo;
		}

		/*!
			Método que altera todas as prioridades da tomada para o valor passado por parâmetro.
			\param p é o novo valor das prioridades da tomada.
		*/
		void setPrioridades(int p) {
			prioridades.madrugada = p;
			prioridades.manha = p;
			prioridades.tarde = p;
			prioridades.noite = p;
		}

		/*!
			Método que altera a prioridade no período das 00:00 às 06:00.
 			\param p é o novo valor da prioridade nesse horário.
		*/
		void setPrioridadeMadrugada(int p) {
			prioridades.madrugada = p;
		}

		/*!
			Método que altera a prioridade no período das 06:00 às 12:00.
 			\param p é o novo valor da prioridade nesse horário.
		*/
		void setPrioridadeManha(int p) {
			prioridades.manha = p;
		}

		/*!
			Método que altera a prioridade no período das 12:00 às 18:00.
 			\param p é o novo valor da prioridade nesse horário.
		*/
		void setPrioridadeTarde(int p) {
			prioridades.tarde = p;
		}

		/*!
			Método que altera a prioridade no período das 18:00 às 00:00.
 			\param p é o novo valor da prioridade nesse horário.
		*/
		void setPrioridadeNoite(int p) {
			prioridades.noite = p;
		}

		/*!
			Método que altera se a tomada pode ou não ser desligada no periodo especificado.
 			\param valor é o valor booleano que especifica se a tomada pode ou não desligar.
 			\param periodo é o periodo em que sera feita a alteração. (0 -Madrugada, 1 -Manhã, 2 -Tarde, 3 -Noite)
		*/
		void setPodeDesligar(bool valor, int periodo) {
			podeDesligar[periodo] = valor;
		}

		/*!
			Método que retorna as prioridades da tomada.
			\return Struct que contém as quatro possiveis prioridades da tomada.
		*/
		Prioridades getPrioridades() {
			return prioridades;
		}

		/*!
			Método que retorna se a tomada pode desligar.
 			\param periodo é o periodo em que se deseja consultar. (0 -Madrugada, 1 -Manhã, 2 -Tarde, 3 -Noite)
			\return Valor booleano que representa se a tomada pode desligar.
		*/
		bool getPodeDesligar(int periodo) {
			return podeDesligar[periodo];
		}

		/*!
			Método que retorna o consumo atual da tomada.
			\return Valor float que indica o consumo atual da tomada. Caso esteja desligada, o valor retornado é 0.
		*/
		virtual float getConsumo() { //mudar o random

			// Método criado para possibilitar a simulação da análise de consumo de uma tomada a cada 6 horas.
			// Em um sistema real este método retornaria o consumo da tomada.

			if (ligada) {
				unsigned int rand;
				if (consumo == 0) {
					rand = Random::random();
					consumo = 200 + (rand % (500-200+1));
				}
				// Aplica uma variação ao último consumo registrado, para simular um sistema real onde os valores são de certa forma consistentes.
				rand = Random::random();
				float variacao = (90 + (rand % 21)) / 100.0f; // Valor de 0.9 até 1.1 ou 90% até 110%

				consumo = (consumo+variacao) * variacao;
			} else {
				consumo = 0;
			}
			return consumo;
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
		TomadaMulti() {
			tipo = 2; // Indica uma TomadaInteligente com dimmer
		}

		/*!
			Método que define o valor da porcentagem de dimmerização da tomada.
		*/
		void setDimerizacao(float porcentagem) {
			dimPorcentagem = porcentagem;
		}

		/*!
			Método que calcula a porcentagem de dimmerização da tomada. Essa porcentagem é armazenada na variável dimPorcentagem.
 			\param consumo é o consumo previsto da tomada até o fim do mês.
 			\param sobra é o máximo de consumo que a tomada pode ter para o limite máximo de consumo ser mantido.
		*/
		void dimerizar(float consumo, float sobra) {
			dimPorcentagem = (sobra/consumo);
		}

		/*!
			Método que retorna o consumo atual da tomada.
			\return Valor float que indica o consumo atual da tomada. Caso esteja desligada, o valor retornado é 0.
		*/
		float getConsumo() {

			// Método criado para possibilitar a simulação da análise de consumo de uma tomada a cada 6 horas.
 			// Em um sistema real este método retornaria o consumo da tomada.
			TomadaInteligente::getConsumo();
			return consumo * dimPorcentagem;

		}
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
			Método estático que estima o consumo da tomada para as pŕoximas 6 horas.
 			\param historico é um vetor que contém os consumos da tomada.
		*/
		static float preverConsumoProprio(float historico[NUMERO_ENTRADAS_HISTORICO]) { //28 entradas corresponde a 7 dias(uma entrada a cada 6 horas)
            float previsao =   (1/406.0) * historico[0]
							+  (2/406.0) * historico[1]
							+  (3/406.0) * historico[2]
							+  (4/406.0) * historico[3]
							+  (5/406.0) * historico[4]
							+  (6/406.0) * historico[5]
							+  (7/406.0) * historico[6]
							+  (8/406.0) * historico[7]
							+  (9/406.0) * historico[8]
							+ (10/406.0) * historico[9]
							+ (11/406.0) * historico[10]
							+ (12/406.0) * historico[11]
							+ (13/406.0) * historico[10]
							+ (14/406.0) * historico[13]
							+ (15/406.0) * historico[14]
							+ (16/406.0) * historico[15]
							+ (17/406.0) * historico[16]
							+ (18/406.0) * historico[17]
							+ (19/406.0) * historico[18]
							+ (20/406.0) * historico[19]
							+ (21/406.0) * historico[20]
							+ (22/406.0) * historico[21]
							+ (23/406.0) * historico[22]
							+ (24/406.0) * historico[23]
							+ (25/406.0) * historico[24]
							+ (26/406.0) * historico[25]
							+ (27/406.0) * historico[26]
							+ (28/406.0) * historico[27];
        	return previsao;
    	}

		/*!
			Método que estima o consumo de todas as tomadas juntas até o fim do mês.
			\param h é uma hash que contém os valores enviados pelas outras tomadas.
 			\param minhaPrevisao é a previsão da tomada até o fim do mês.
			\return Valor previsto para o consumo total das tomadas.
		*/
		static float preverConsumoTotal(Tabela* h, float minhaPrevisao) {
			float total = minhaPrevisao;
			for(auto iter = h->begin(); iter != h->end(); iter++) {
				//se iter não é vazio: begin() retorna um objeto vazio no inicio por algum motivo
				if (iter != 0) {
					total += iter->object()->consumoPrevisto;
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
		Relogio* relogio; /*!< Objeto que possui informações como data e hora.*/
		Mensageiro* mensageiro;	/*!< Objeto que provê a comunicação da placa com as outras.*/
		Tabela* hash; /*!< Hash que guarda informações recebidas sobre as outras tomadas indexadas pelo endereço da tomada.*/
		float maximoConsumoMensal; /*!< Variável que indica o máximo de consumo que as tomadas podem ter mensalmente.*/
		float consumoMensal; /*!< Variável que indica o consumo mensal das tomadas até o momento.*/
		float consumoProprioPrevisto; /*!< Variável que indica o consumo previsto da tomada no mês.*/
		float consumoTotalPrevisto; /*!< Variável que indica o consumo total previsto no mês.*/
		float *historico; /*!< Vetor que guarda o consumo da tomada a cada 6 horas.*/
		int quantidade6Horas; /*!< Variável que indica a quantidade de quartos de dia(6 horas) que faltam para o fim do mês.*/
		float consumoProprio; /*!< Variável que indica o consumo da tomada no último período.*/


		/*!
			Método que realiza o trabalho da placa, fazendo sua previsão, sincronização e ajustes no estado da tomada conforme o necessário.
			\sa calculaQuantidadeQuartosDeDia(), atualizaHistorico(), fazerPrevisaoConsumoProprio(), preparaEnvio(), sincronizar(),
 				atualizaConsumoMensal(), fazerPrevisaoConsumoTotal(), administrarConsumo()
		*/
		void administrar() {

			// Entrando em um novo mês
			if (quantidade6Horas == 0) {
				consumoMensal = 0;
				calculaQuantidadeQuartosDeDia();
			}

			cout << "- Previsao." << endl;
			// Preparando a previsao própria.
			float ultimoConsumo = consumoProprio;
			cout << "  Consumo efetivo das ultimas 6h: " << ultimoConsumo << endl;
			atualizaHistorico(ultimoConsumo);
			fazerPrevisaoConsumoProprio();
			cout << "  Previsao propria ate o fim do mes: " << consumoProprioPrevisto << endl;

			cout << "- Prepadando dados para enviar." << endl;
			// Preparando Dados para enviar.
			Dados dadosEnviar;
			dadosEnviar = preparaEnvio();

			cout << "- Entrando em sincronizacao." << endl;
			// Sincronização entre as placas.
			sincronizar(dadosEnviar);
			cout << "  Placas sincronizadas." << endl;

			cout << "- Dados obtidos:" << endl;
			printHash();

			cout << "- Tomada de decisao:" << endl;
			// Atualiza as previsões com base nos novos dados recebidos.
			atualizaConsumoMensal();
			fazerPrevisaoConsumoTotal(); // Considera todas as tomadas, mesmo as desligadas.
			cout << "  Consumo efetivo deste mes ate o momento: ............. " << consumoMensal << endl;
			cout << "  Consumo maximo permitido ate o final do mes: ......... " << maximoConsumoMensal << endl;
			cout << "  Consumo total previsto do sistema ate o fim do mes: .. " << (consumoTotalPrevisto+consumoMensal) << endl;
			cout << "  Prioridade atual: .................................... " << dadosEnviar.prioridade << endl;

			// Toma decisões dependendo de como está o consumo do sistema.
			administrarConsumo();

			// Atualiza a variável de controle que indica quantas verificações ainda serão feitas dentro desse mês.
			quantidade6Horas--;
			consumoProprio = 0;
		}

		/*!
			Método que prepara os dados a serem enviados para as outras tomadas.
			\return Dados a serem enviados com os valores atuais da tomada.
 			\sa prioridadeAtual()
		*/
		Dados preparaEnvio() {
			Dados dados;
			dados.remetente = mensageiro->obterEnderecoNIC();
			//dados.ligada = tomada->estaLigada();
			dados.consumoPrevisto = consumoProprioPrevisto;
			if (tomada->estaLigada()) {
				dados.ultimoConsumo = historico[NUMERO_ENTRADAS_HISTORICO - 1];
			} else {
				dados.ultimoConsumo = 0;
			}

			dados.configuracao[0] = '\0';
			dados.prioridade = prioridadeAtual();
			dados.podeDesligar = podeDesligarAtual();
			return dados;
		}

		/*!
			Método que sincroniza as placas, enviando e recebendo mensagens com dados.
			\param dadosEnviar é a struct que contém os dados que esta placa estará enviando.
			\sa enviarMensagemBroadcast(), atualizaHash()
		*/
		void sincronizar(Dados dadosEnviar) {
			Dados* dadosRecebidos;
			Hash_Element* elemento; // Elemento da hash.

			Chronometer* cronSinc = new Chronometer();

			long long tempoDeSinc = INTERVALO_ENVIO_MENSAGENS*60*1000000; // Minutos pra Microssegundos.
			long long nextSend = 0;
			long long nextReceive = 0;
			cronSinc->reset();
			cronSinc->start();
			long long cronTime = cronSinc->read();

			while (cronTime < tempoDeSinc) {
				if (cronTime >= nextSend) {
					enviarMensagemBroadcast(dadosEnviar);
					nextSend += tempoDeSinc/15; // 15 envios durante a sincronização.
				} else if (cronTime >= nextReceive) {
					dadosRecebidos = receberMensagem();
					elemento = new Hash_Element(dadosRecebidos,dadosRecebidos->remetente); // Hash é indexada pelo endereço da tomada.
					atualizaHash(elemento);
					nextReceive += tempoDeSinc/300; // Recebe 300 vezes durante a sincronização.
				}
				cronTime = cronSinc->read();
			}
			delete cronSinc;
		}

		/*!
			Método que verifica se consumo previsto está acima do m´sximo e se alguma decisão deve ser tomada.
 			\sa mantemConsumoDentroDoLimite()
		*/
		void administrarConsumo() {
			// Se o consumo até agora somado à previsão de consumo até o fim do mês ficam acima do consumo máximo.
			if (consumoMensal + consumoTotalPrevisto > maximoConsumoMensal) {
				cout << "  A previsao passa do limite." << endl;
				mantemConsumoDentroDoLimite(); // Desliga as tomadas necessárias para manter o consumo dentro do limite.
			} else { // Se o consumo está dentro do limite
				cout << "  A previsao esta dentro do limite. Posso ligar." << endl;
				// Liga todas as tomadas
				if (tomada->getTipo() == 2) {
					tomada->ligar();
					static_cast<TomadaMulti*>(tomada)->setDimerizacao(1);
				} else {
					tomada->ligar();
				}
			}
		}

		/*!
			Método que atualiza o vetor de histórico de consumo da tomada com o novo consumo. Consumo nulo(tomada desligada) não é inserido.
			\param novo é o consumo atual da tomada que será inserido no histórico.
		*/
		void atualizaHistorico(float novo) {
			if (tomada->estaLigada()) {
				int i;
				for (i = 0; i < (NUMERO_ENTRADAS_HISTORICO - 1); i++) {
					historico[i] = historico[i+1];
				}
				// Consumo novo é adicionado no fim do vetor para manter coerência com a lógica da previsão de consumo
				historico[NUMERO_ENTRADAS_HISTORICO - 1] = novo;
			}
		}

		/*!
			Método que atualiza a entrada da hash correspondente ao elemente passado por parâmetro. Se ele não existir, é adicionado.
			\param e é o elemento a ser atualizado ou adicionado na hash.
		*/
		void atualizaHash(Hash_Element* e) {
			Hash_Element* foundElement = hash->search_key(e->object()->remetente);
			if (foundElement != 0) {  // Se uma entrada para a tomada passada já existe

				/*
				Dados* d =  foundElement->object();
				//d->ligada = e->object()->ligada;
				d->consumoPrevisto = e->object()->consumoPrevisto;
				d->ultimoConsumo = e->object()->ultimoConsumo;
				d->prioridade = e->object()->prioridade;
				d->configuracao = e->object()->configuracao;
				d->podeDesligar = e->object()->podeDesligar;
				delete e->object();
				delete e;
				*/

				delete foundElement->object();
				delete foundElement;
				hash->insert(e);
			} else if (e->object()->prioridade != -1) {
				// Se elemento não está na hash e não é um elemento "vazio"(prioridade é igual a -1 quando não há mensagem recebida)
				hash->insert(e);
			} else if (e->object()->prioridade == -1) {
				delete e->object();
				delete e;
			}
		}

		/*!
			Método que calcula quanta energia já foi consumida no mês e atualiza a variável global consumoMensal.
		*/
		void atualizaConsumoMensal() {
			if (tomada->estaLigada()) {
				consumoMensal += historico[NUMERO_ENTRADAS_HISTORICO - 1];
			}
			for(auto iter = hash->begin(); iter != hash->end(); iter++) {
				// Se iter não é vazio: begin() retorna um objeto vazio no inicio por algum motivo
				if (iter != 0) {
					consumoMensal += iter->object()->ultimoConsumo;
				}
			}
		}

		/*!
			Método que inicializa todos os valores do histórico para 0.
		*/
		void inicializarHistorico() {
			for (int i = 0; i < NUMERO_ENTRADAS_HISTORICO; i++) {
				historico[i] = 0;
			}
		}

	public:

		/*!
			Método construtor da classe.
 			\param t é a tomada a ser controlada.
 			\sa inicializarHistorico(), calculaQuantidadeQuartosDeDia()
		*/
		Gerente(TomadaInteligente* t) {
			tomada = t;
			relogio =  new Relogio();
			mensageiro = new Mensageiro();
			hash = new Tabela();

			maximoConsumoMensal = 1000; //consumo máximo padrão

			consumoMensal = 0;
			consumoProprio = 0;
			consumoProprioPrevisto = 0;
			consumoTotalPrevisto = 0;

			historico = new float[NUMERO_ENTRADAS_HISTORICO];
			inicializarHistorico();

			calculaQuantidadeQuartosDeDia();
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
			\return Ponteiro de Dados que indica os dados recebidos.
		*/
		Dados* receberMensagem() {
			return mensageiro->receberMensagem();
		}

		/*!
			Método que atualiza o valor da previsão do consumo da tomada até o fim do mês. O valor é armazenado na variável global consumoProprioPrevisto.
			\sa Previsor
		*/
		void fazerPrevisaoConsumoProprio() {
			/*Cada previsão é para as próximas 6 horas. Assim, esse valor é multiplicado por quantas mais 6 horas faltam para acabar o mês para depois sabermos se o consumo está dentro do limite.*/
			float retorno = Previsor::preverConsumoProprio(historico);
			consumoProprioPrevisto = retorno * quantidade6Horas;
		}

		/*!
			Método que atualiza o valor da previsão do consumo total das tomadas até o fim do mês. O valor é armazenado na variável global consumoTotalPrevisto.
		*/
		void fazerPrevisaoConsumoTotal() {
			consumoTotalPrevisto = Previsor::preverConsumoTotal(hash, consumoProprioPrevisto);
		}

		/*!
			Método que realiza a sincronização entre as tomadas a cada 6 horas.
			\sa administrar(), pausa()
		*/
		void iniciar() { //menos tempo entre as sincronizações
			Chronometer* cronPrincipal = new Chronometer();
			Data data = relogio->getData();

			// Tempo que passou desde o último quarto de dia
			long long tempoQuePassou = ((data.hora*60*60 + data.minuto*60 + data.segundo) % (6*60*60)) * 1000000;
			long long timeTillNextWake;
			long long milisecsEm6Horas = 6*60*60*1000000.0;

			cout << "Iniciando Sistema" << endl << endl;

			while (true) {
				timeTillNextWake = milisecsEm6Horas - tempoQuePassou;

				cout << "Pausa entre sincronizacoes." << endl << endl;

				pausa(timeTillNextWake);

				cout << "Acordando." << endl;

				cronPrincipal->reset();
				cronPrincipal->start();

				administrar();

				cronPrincipal->stop();
				tempoQuePassou = cronPrincipal->read();

			}
		}

		/*!
			Método que, baseado no horário atual, descobre a prioridade certa.
			\return Valor da prioridade da tomada no período atual do dia.
		*/
		int prioridadeAtual() {
			Prioridades prioridades = tomada->getPrioridades();
			Data data = relogio->getData();
			long long hora = data.hora;
			int quartosDeDia = (int) hora / 6;
			switch(quartosDeDia){
				case 0:
					return prioridades.madrugada;
				case 1:
					return prioridades.manha;
				case 2:
					return prioridades.tarde;
				case 3:
					return prioridades.noite;
			}
		}

		/*!
			Método que, baseado no horário atual, descobre se a tomada pode desligar.
			\return Valor booleano que especifica se a tomada pode desligar.
		*/
		int podeDesligarAtual() {
			Data data = relogio->getData();
			long long hora = data.hora;
			int quartosDeDia = (int) hora / 6;
			return tomada->getPodeDesligar(quartosDeDia);
		}

		/*!
			Método que verifica se a tomada deve desligar para manter o consumo mensal dentro do consumo máximo.
		*/
		void mantemConsumoDentroDoLimite() {
			// Representa o quanto de consumo ainda resta até atingir o limite do mês.
			float consumoRestante = maximoConsumoMensal - consumoMensal;

			float sobraDeConsumo = 0;
			float diferencaConsumo;

			// É o consumo total de todas as tomadas de menor prioridade que esta.
			float consumoInferiores = 0;
			// É o consumo total de todas as tomadas de mesma prioridade.
			float consumoMesmaPioridade = consumoProprioPrevisto;
			// É o consumo total de todas as tomadas de mesma prioridade e de consumo inferior.
			float menorConsumoMesmaPrioridade = 0;
			// Indica se há outras tomadas com a mesma prioridade.
			bool outrasComMesmaPrioridade = false;

			for(auto iter = hash->begin(); iter != hash->end(); iter++) {
				// Se iter não é vazio: begin() retorna um objeto vazio no inicio por algum motivo
				if (iter != 0) {
					if(prioridadeAtual() > iter->object()->prioridade) { // Outras tomadas que têm prioridade abaixo da minha prioridade.
						consumoInferiores += iter->object()->consumoPrevisto;
					} else if (prioridadeAtual() == iter->object()->prioridade) { // Outras tomadas com a mesma prioridade.
						outrasComMesmaPrioridade = true;
						consumoMesmaPioridade += iter->object()->consumoPrevisto;
						if (consumoProprioPrevisto > iter->object()->consumoPrevisto) { // Tomadas cujo consumo é menor.
							menorConsumoMesmaPrioridade += iter->object()->consumoPrevisto;
						}
					}
				}
			}


			diferencaConsumo = consumoTotalPrevisto - consumoInferiores;
			if (diferencaConsumo <= consumoRestante) { // Se desligando as tomadas com prioridade inferior o consumo já fica dentro do limite.
				cout << "   Desligando as tomadas de prioridade inferior o consumo fica abaixo do maximo. Posso ficar ligada." << endl;
				// Liga a tomada.
				if (tomada->getTipo() == 2) {
					cout << "    Dimerizo para 100%." << endl;
					tomada->ligar();
					static_cast<TomadaMulti*>(tomada)->setDimerizacao(1);
				} else {
					tomada->ligar();
				}
			} else { // Se mesmo desligando as tomadas com prioridade inferior o consumo ainda está abaixo do limite
				cout << "   Desligando as tomadas de prioridade inferior o consumo ainda fica acima do maximo. Sou uma candidata a ser desligada." << endl;
				if (outrasComMesmaPrioridade) { // Se há tomadas com a mesma prioridade.
					// Se desligando as tomadas de mesmo consumo completamente o consumo fica abaixo do necessário para o consumo máximo ser mantido
					cout << "    Ha outras tomadas de mesma prioridade, talvez nao precise desligar." << endl;
					if ((diferencaConsumo - consumoMesmaPioridade) < consumoRestante) {
						// Nesse caso, desligando apenas algumas tomadas de mesma prioridade que essa o consumo ainda pode ser mantido abaixo do limite
						// Aqui, as tomadas com consumo menor são as escolhidas para desligamento
						if ((diferencaConsumo - menorConsumoMesmaPrioridade)  <= consumoRestante) { // Se desligando as tomadas com mesma prioridade mas consumo menor é o suficiente para manter o limite de consumo
							cout << "     Desligando as outras tomadas de mesma prioridade e que consomem menos o consumo fica abaixo do maximo, posso ligar." << endl;
							// Essa tomada pode ser ligada
							if (tomada->getTipo() == 2) {
								cout << "      Dimerizo para 100%." << endl;
								tomada->ligar();
								static_cast<TomadaMulti*>(tomada)->setDimerizacao(1);
							} else {
								tomada->ligar();
							}
						} else { //Tomada é desligada ou dimerizada
							cout << "     Mesmo desligando todas as tomadas de mesma prioridade e que consomem menos, o consumo ainda fica acima do maximo, devo desligar." << endl;
						tomada->desligar();
							if ((diferencaConsumo - menorConsumoMesmaPrioridade - consumoProprioPrevisto)  <= consumoRestante && tomada->getTipo() == 2) {
								// Se pode dimerizar
								sobraDeConsumo = consumoRestante - (diferencaConsumo - menorConsumoMesmaPrioridade - consumoProprioPrevisto);
								tomada->ligar();
								static_cast<TomadaMulti*>(tomada)->dimerizar(consumoProprioPrevisto, sobraDeConsumo);
								cout << "      Dimerizo para " << (static_cast<TomadaMulti*>(tomada)->getPorcentagem()*100) << "%." << endl;
							} else {
								cout << "      Desligo." << endl;
								tomada->desligar();
							}
						}

					} else { // Se mesmo desligando todas as tomadas de mesma prioridade o consumo ainda estiver acima do limite, não há o que fazer, apenas desligar.
						cout << "     Mesmo desligando todas as tomadas de mesma prioridade, o consumo ainda fica acima do maximo, devo desligar." << endl;
						tomada->desligar();
					}
				} else { // Se não há outras tomadas com a mesma prioridade.
					 // Se desligando a tomada completamente o consumo fica abaixo do necessário para o consumo máximo ser mantido e a tomada tem dimmer
					cout << "    Nao ha outras tomadas de mesma prioridade, devo ser desligada." << endl;
					if ((diferencaConsumo - consumoProprioPrevisto) < consumoRestante && tomada->getTipo() == 2) {
						sobraDeConsumo = consumoRestante - (diferencaConsumo - consumoProprioPrevisto);
						tomada->ligar();
						static_cast<TomadaMulti*>(tomada)->dimerizar(consumoProprioPrevisto, sobraDeConsumo);
						cout << "     Dimerizo para " << (static_cast<TomadaMulti*>(tomada)->getPorcentagem()*100) << "%." << endl;
					} else { // Se não tem dimmer
						cout << "     Desligo." << endl;
						tomada->desligar();
					}
				}
			}
		}

		/*!
			Método que calcula quantos 1/4 de dia faltam para o fim do mês. O valor é armazenado na variável global quantidade6Horas.
			\sa diasRestantes()
		*/
		void calculaQuantidadeQuartosDeDia() {
			// +1 para contar o dia de hoje e podermos subtrair os quartos que já passaram.
			quantidade6Horas = (diasRestantes()+1) * 4;
			int hora = relogio->getData().hora;
			// Ajusta da quantidade para quando a tomada é criada depois do primeiro 1/4 do dia.
			int quartoDoDia = (int) hora / 6;
			quantidade6Horas = quantidade6Horas - quartoDoDia;
		}

		/*!
			Método que calcula quantos dias restam para o mês acabar.
			\return Valor inteiro que indica quantos dias faltam para o fim do mês.
		*/
		int diasRestantes() {
			Data data = relogio->getData();
			int diaAtual = data.dia;
			int diasNoMes = relogio->getDiasNoMes(data.mes, data.ano);
			return (diasNoMes - diaAtual);
		}

		/*!
			Método em que a placa soma seu consumo nos períodos entre sincronizações.
 			\param microssegundos é o tempo em microssegundos que se deseja esperar até a próxima sincronização.
		*/
		void pausa(long long microssegundos){
			while (microssegundos > 0) {
				RTC::Microsecond timeSlept = (RTC::Microsecond) microssegundos;
				Alarm::delay(timeSlept);
				relogio->atualizaRelogio();
				microssegundos -= timeSlept;
			}
			// consumoProprio soma getConsumo
		}

		/*!
			Método que trata de alterações na configuração da tomada através de mensagens USB.
		*/
		void configuracaoViaUSB() {
			if (temMsgUSB()) {
				char strReceived[NUMERO_CHAR_CONFIG];
				receberConfigViaUSB(strReceived);
				processarComando(strReceived, true);
			}
		}

		/*!
			Método que verifica se há alguma mensagem de configuração sendo transmitida por USB.
			\return Valor booleano que representa se hà uma mensagem via USB ou não.
		*/
		bool temMsgUSB() {
			return USB::ready_to_get();
		}

		/*!
			Método que recebe mensagens de configuração via USB.
			\param receivedStr é o char* que irá receber a mensagem.
		*/
		void receberConfigViaUSB(char* receivedStr) {

			// Para garantir que não vai ter lixo na mensagem.
			for (int i; i < NUMERO_CHAR_CONFIG; i++) {
				receivedStr[i] = '\0';
			}

			if (temMsgUSB()) {
				int cont = 0;
				while (temMsgUSB()) {
					char c = USB::get();
					receivedStr[cont] = c;
					cont++;
				}
			} else {
				receivedStr = "NOCON";
			}
		}

		/*!
			Método que executa os comandos de configuração.
			\param comando é o comando que será executado.
		*/
		void processarComando(char* comando, bool reenviar) {

			bool todos = false;
			bool souAlvo = false;

			char destinoHex[5];
			for (int i; i < 5; i++) {
				destinoHex[i] = comando[i];
			}
			destinoHex[5] = '\0';

			char destinoDec[7];
			mensageiro->converterEndereco(destinoHex, destinoDec);
			Address* addDestino = new Address(destinoDec);
			Address meuAdd = mensageiro->obterEnderecoNIC();



			if (*addDestino == meuAdd) {
				souAlvo = true;
			} else if (strcmp(destinoHex, "TODAS") == 0) {
				souAlvo = true;
				todos = true;
			}

			if (souAlvo) {
				char cmd[7];
				for (int i; i < 7; i++) {
					cmd[i] = comando[i+6];
				}
				cmd[7] = '\0';

				cout << cmd << endl;

				if (strcmp(cmd, "PRIORID") == 0) {
					char periodo[3];
					for (int i; i < 3; i++) {
						periodo[i] = comando[i+14];
					}
					periodo[3] = '\0';

					int prioridade = strToNum(comando+18);

					if (prioridade > 0){
						if (strcmp(periodo, "MAD") == 0) {
							tomada->setPrioridadeMadrugada(prioridade);
						} else if (strcmp(periodo, "MAN") == 0) {
							tomada->setPrioridadeManha(prioridade);
						} else if (strcmp(periodo, "TAR") == 0) {
							tomada->setPrioridadeTarde(prioridade);
						} else if (strcmp(periodo, "NOI") == 0) {
							tomada->setPrioridadeNoite(prioridade);
						}
					}

				} else if (strcmp(cmd, "DESLIGA") == 0) {
					char periodo[3];
					for (int i; i < 3; i++) {
						periodo[i] = comando[i+14];
					}
					periodo[3] = '\0';

					char valorStr[3];
					for (int i; i < 3; i++) {
						valorStr[i] = comando[i+18];
					}
					valorStr[3] = '\0';

					bool valor;
					if (strcmp(valorStr, "FALSE") == 0) {
						valor = false;
					} else {
						valor = true;
					}

					if (strcmp(periodo, "MAD") == 0) {
						tomada->setPodeDesligar(valor, 0);
					} else if (strcmp(periodo, "MAN") == 0) {
						tomada->setPodeDesligar(valor, 1);
					} else if (strcmp(periodo, "TAR") == 0) {
						tomada->setPodeDesligar(valor, 2);
					} else if (strcmp(periodo, "NOI") == 0) {
						tomada->setPodeDesligar(valor, 3);
					}
				} else if (strcmp(cmd, "RELOGIO") == 0) {

				} else if (strcmp(cmd, "CONSUMO") == 0) {
					char* s = comando + 14;
					long long int consumo = strToNum(s);
					maximoConsumoMensal = (float) consumo;
				} else {

				}
			}

			if (reenviar && (todos || !souAlvo)) {
				// Não interessa pra mim, e reenvio esta habilitado
				// Reenviar.
			}


			cout << maximoConsumoMensal << endl;
			cout << tomada->getPrioridades().madrugada << endl;
			cout << tomada->getPrioridades().manha << endl;
			cout << tomada->getPrioridades().tarde << endl;
			cout << tomada->getPrioridades().noite << endl;

		}

		long long int strToNum(char* str) {
			long long int num = 0;
			int cont = 0;
			int proxNum = (*str-'0');
			while ((proxNum >= 0) && (proxNum <= 9)) {
				num *= 10;
				num += proxNum;
				str++;
				proxNum = (*str-'0');
				cont++;
			}
			return num;
		}

		// Metodo para testes
		void printHash() {
			for(auto iter = hash->begin(); iter != hash->end(); iter++) {
				if (iter != 0) {
					Dados* d = iter->object();
					cout << "   Placa " << d->remetente << ":" << endl;
					cout << "    Consumo previsto: .. " << d->consumoPrevisto << endl;
					cout << "    Ultimo consumo: .... " << d->ultimoConsumo << endl;
					cout << "    Prioridade: ........ " << d->prioridade << endl;
				}
			}
		}
};

//----------------------------------------------------------------------------
//!  Método Main
/*!
	Método inicial do programa.
*/
int main() {

	// Para instanciar uma tomada inteligente com dimmer, utilize TomadaMulti ao invés de TomadaInteligente.
	TomadaInteligente* t = new TomadaInteligente();

	// Aqui instanciamos o Relógio que permite capturar a passagem do tempo.
	// Ele será inicializado no horário que for passado à ele no construtor.
	// Seus parâmetros são o ano, o mês, o dia, a hora e o minuto, respectivamente.
	//Relogio* r = new Relogio(2016,10,1,05,59);
	//Gerente* g = new Gerente(20000, t, r);
	Gerente* g = new Gerente(t);

	// Estes métodos definem as prioridades da tomada ao longo do dia.
	t->setPrioridadeMadrugada(5);
	t->setPrioridadeManha(5);
	t->setPrioridadeTarde(5);
	t->setPrioridadeNoite(5);

	// DEBUG
	while (true) {
		g->configuracaoViaUSB();
	}
	// /DEBUG


	// Aqui é iniciado o sistema.
	g->iniciar();


	while (true);
};
