// Copyright [2016] <Dúnia Marchiori(14200724) e Vinicius Steffani Schweitzer(14200768)>

#include <gpio.h>
#include <nic.h>
#include <utility/hash.h>
#include <utility/random.h>
#include <alarm.h>
#include <chronometer.h>


#define INTERVALO_ENVIO_MENSAGENS 5 /*!< Intervalo(em minutos) em que são feitos envio e recebimento de mensagens entre as tomadas. */
#define NUMERO_ENTRADAS_HISTORICO 28 /*!< Quantidade de entradas no histórico. Cada entrada corresponde ao consumo a cada 6 horas. */

using namespace EPOS;

OStream cout;

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
	float ultimoConsumo; /*!< Corresponde ao valor do consumo da tomada nas últimas 6 horas. */
    int prioridade; /*!< Corresponde à prioridade da tomada. */
};

//!  Struct Data
/*!
	Struct contendo valores de uma data.
*/
struct Data {
	unsigned long long dia; /*!< Variável que representa o dia atual.*/
	unsigned long long mes; /*!< Variável que representa o mês atual.*/
	unsigned long long ano; /*!< Variável que representa o ano atual.*/
	unsigned long long hora; /*!< Variável que representa a hora atual.*/
	unsigned long long minuto; /*!< Variável que representa os minutos atuais.*/
	unsigned long long segundo; /*!< Variável que representa os segundos atuais.*/
	unsigned long long microssegundos; /*!< Variável que representa os microssegundos atuais.*/
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
		Dados* receberMensagem() {
			Address* remetente;
			Protocol* prot;
			Dados* msg = new Dados();
			bool hasMsg = nic->receive(remetente, prot, msg, sizeof *msg);
			
			if (!hasMsg) { // Se não foi recebida nenhuma mensagem
				msg->remetente = -1;
				msg->ligada = 0;
				msg->consumoPrevisto = -1;
				msg->ultimoConsumo = -1;
				msg->prioridade = -1;
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
};

//----------------------------------------------------------------------------
//!  Classe Relogio
/*!
	Classe que lida com as informações de datas e horarios.
*/
class Relogio {
	private:
		Data data;
		int diasNoMes[12]; /*!< Vetor que guarda quantos dias tem em cada mês.*/
		Chronometer* cronometro;

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
			Método construtor da classe
		*/
		Relogio(unsigned int a, unsigned int m, unsigned int d, unsigned int h, unsigned int min) {
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
		}
		
		/*!
			Método que retorna o dia atual.
			\return um inteiro que representa o dia atual.
		*/
		Data getData() {
			atualizaRelogio();
			return data;
		}

		/*!a
			Método que atualiza os dados do relogio, baseado no tempo desde a ultima requisicao.
		*/
		void atualizaRelogio() {
			unsigned long long tempoDecorrido = cronometro->read();
			cronometro->reset();
			cronometro->start();
			incrementarMicrossegundo(tempoDecorrido);
		}
		
		/*!
			Método que incrementa o ano.
			\param anos Quantos anos serão incrementados no relogio.
		*/
		void incrementarAno(long long anos){
			data.ano+=anos;
		}
		
		/*!
			Método que incrementa os meses de forma a atualizar todas as unidades de tempo superiores.
			\param meses Quantos meses serão incrementados no relogio.
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
			\param dias Quantos dias serão incrementados no relogio.
		*/
		void incrementarDia(long long dias){
			while (dias > 0) {
				data.dia++;
				if (data.dia > getDiasNoMes()) {
					incrementarMes(1);
					data.dia = 1;
				}
				dias--;
			}
		}
		
		/*!
			Método que incrementa as horas de forma a atualizar todas as unidades de tempo superiores.
			\param hrs Quantas horas serão incrementados no relogio.
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
			\param mins Quantos minutos serão incrementados no relogio.
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
			\param secs Quantos segundos serão incrementados no relogio.
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
			\param mcsec Quantos microssegundos serão incrementados no relogio.
		*/
		void incrementarMicrossegundo(long long mcsec){
			data.microssegundos+=mcsec;
			if (data.microssegundos >= 1000000) {
				incrementarSegundo( data.microssegundos/1000000 );
				data.microssegundos = data.microssegundos % 1000000;
			}
		}
		
		/*!
			Método que retorna quantos dias tem no mes atual.
			\return Valor inteiro que representa quantos dias tem no mês.
		*/
		int getDiasNoMes() {
			Data d = getData();
			if (d.ano % 4 == 0 && d.mes == 2) { //se é ano bissexto e o mês é fevereiro
				return 29;
			} else {
				return diasNoMes[d.mes-1];
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
			Método que retorna a a porcentagem de dimmerização da tomada.
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
		int tipo; /*!< Variável que indica o tipo da tomada.*/
	private:
		Prioridades prioridades; /*!< Variável que contém as prioridade da tomada ao longo do dia.*/
		float consumo; /*!< Variável que indica o consumo da tomada.*/

	public:
		/*!
			Método construtor da classe
		*/
		TomadaInteligente() {
			consumo = 0;
			tipo = 1; //indica uma TomadaInteligente
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
			
			// Metodo criado para simular a analise de consumo de uma tomada a cada 6 horas.
			// Em um sistema real este metodo retornaria o consumo da tomada.
				
			if (ligada) {
				unsigned int rand;
				if (consumo == 0) {
					rand = Random::random();
					consumo = 200 + (rand % (500-200+1));
				}
				// Aplica uma variacao ao ultimo consumo registrado, para simular um sistema real onde os valores são de certa forma consistentes.
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
			Método que calcula a porcentagem de dimmerização da tomada.
			\param p é a porcentagem de dimmerização.
		*/
		void dimmerizar(float consumo, float sobra) {
			dimPorcentagem = (sobra/consumo);
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
			Método que estima o consumo da tomada para as pŕoximas 6 horas.
			\return Valor previsto para o consumo da tomada.
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
			\param quartosDeDia é a quantidade de quartos de dia(6 horas) que faltam para o mês acabar.
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
		float maximoConsumoMensal; /*!< Variável que indica o máximo de consumo que as tomadas podem ter mensalmente.*/
		float consumoMensal; /*!< Variável que indica o consumo mensal das tomadas até o momento.*/
		float consumoProprioPrevisto; /*!< Variável que indica o consumo previsto da tomada no mês.*/
		float consumoTotalPrevisto; /*!< Variável que indica o consumo total previsto no mês.*/
		float *historico; /*!< Vetor que guarda o consumo da tomada a cada 6 horas.*/
		int quantidade6Horas; /*!< Variável que indica a quantidade de quartos de dia(6 horas) que faltam para o fim do mês.*/
		Relogio* relogio; /*!< Objeto que possui informações como data e hora.*/
		Mensageiro* mensageiro;	/*!< Objeto que provê a comunicação da placa com as outras.*/
		Tabela* hash; /*!< Hash que guarda informações recebidas sobre as outras tomadas indexadas pelo endereço da tomada.*/
	

		/*!
			Método que realiza o trabalho da placa, fazendo sua previsão, sincronização e ajustes no estado da tomada conforme o necessário.
		*/
		void administrar() {
			
			// Entrando em um novo mes
			if (quantidade6Horas == 0) { 
				consumoMensal = 0;
				calculaQuantidadeQuartosDeDia();
			}
			
			// Preparando a previsao propria.
			atualizaHistorico(tomada->getConsumo());
			fazerPrevisaoConsumoProprio();
			
			// Preparando Dados para enviar.
			Dados dadosEnviar;
			dadosEnviar = preparaEnvio();
			
			// Sincronização entre as placas.
			sincronizar(dadosEnviar);
			
			// Atualiza as previsão baseado nos novos dados recebidos.
			atualizaConsumoMensal();
			fazerPrevisaoConsumoTotal(); // Considera todas as tomadas, mesmo as desligadas.
			
			// Toma decisões dependendo de como esta o consumo do sistema.
			administrarConsumo();
			
			// Atualiza a variavel de controle que indica quantas verificações ainda serão feitas dentro desse mês.
			quantidade6Horas--;
		}
		
		
		/*!
			Método que prepara os dados a serem enviados para as outras tomadas.
			\return Ponteiro para os Dados a serem enviados, com seus valores ajeitados.
		*/
		Dados preparaEnvio() {
			Dados dados;
			dados.remetente = mensageiro->obterEnderecoNIC();
			dados.ligada = tomada->estaLigada();
			dados.consumoPrevisto = consumoProprioPrevisto;
			dados.ultimoConsumo = historico[NUMERO_ENTRADAS_HISTORICO - 1];
			dados.prioridade = prioridadeAtual();
			return dados;
		}
		
		
		/*!
			Método que sincroniza as placas, enviando e recebendo mensagens com dados.
			\param dadosEnviar é a struct que contém os dados que esta placa estará enviando.
		*/
		void sincronizar(Dados dadosEnviar) {
			Dados* dadosRecebidos;
			Hash_Element* elemento; // Elemento da hash.
			
			Chronometer* cronSinc = new Chronometer();

			long long tempoDeSinc = INTERVALO_ENVIO_MENSAGENS*60*1000000; // Minutos pra Microssegundos.
			long long nextSend = 0;
			cronSinc->reset();
			cronSinc->start();
			long long cronTime = cronSinc->read();
			
			while (cronTime < tempoDeSinc) {
				if (cronTime > nextSend) {
					enviarMensagemBroadcast(dadosEnviar);
					nextSend += 30000000; // 30 segundos.
				} else {
					dadosRecebidos = receberMensagem();
					elemento = new Hash_Element(dadosRecebidos,dadosRecebidos->remetente); // Hash é indexada pelo endereço da tomada.
					atualizaHash(elemento);
				}
				cronTime = cronSinc->read();
			}
			
			delete cronSinc;
		}
		
		
		/*!
			Método que verifica se consumo previsto esta acima do maximo e se alguma decisao deve ser tomada.
		*/
		void administrarConsumo() {
			if (consumoTotalPrevisto > (maximoConsumoMensal - consumoMensal)) { // Se a previsão está acima do consumo máximo.
				mantemConsumoDentroDoLimite(); // Desliga as tomadas necessárias para manter o consumo dentro do limite.
			} else { // se a previsão está dentro do consumo máximo
				//liga todas as tomadas
				tomada->ligar();			
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
				//consumo novo é adicionado no fim do vetor para manter coerência com a lógica da previsão de consumo
				historico[NUMERO_ENTRADAS_HISTORICO - 1] = novo;
			}			
		}
	
		/*!
			Método que atualiza a entrada da hash correspondente ao elemente passado por parâmetro. Se ele não existir, é adicionado.
			\param e é o elemento a ser atualizado ou adicionado na hash.
		*/
		void atualizaHash(Hash_Element* e) {
			Dados* d =  hash->search_key(e->object()->remetente)->object();
			if (d != 0) {
					d->ligada = e->object()->ligada;
					d->consumoPrevisto = e->object()->consumoPrevisto;
					d->ultimoConsumo = e->object()->ultimoConsumo;
					d->prioridade = e->object()->prioridade;
			} else if(e->object()->remetente != (Address)-1) { 
				//elemento não está na hash e não é um elemento "vazio"(remetente é igual a -1 quando não há mensagem recebida)
				hash->insert(e);	
			}
		}
	
		/*!
			Método que calcula quanta energia já foi consumida no mês e atualiza a variável global consumoMensal.
		*/
		void atualizaConsumoMensal() {
			for(auto iter = hash->begin(); iter != hash->end(); iter++) {
				// Se iter não é vazio: begin() retorna um objeto vazio no inicio por algum motivo
				if (iter != 0) {
					consumoMensal += iter->object()->ultimoConsumo;
				}
			}			
		}

	public:
		/*!
			Método construtor da classe
		*/
		Gerente(TomadaInteligente* t, Relogio* r) {
			relogio = r;
			tomada = t;
			calculaQuantidadeQuartosDeDia();
			consumoMensal = 0;
			mensageiro = new Mensageiro();
			hash = new Tabela();
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
			Método que atualiza o valor da previsão do consumo total das to até o fim do mês. O valor é armazenado na variável global consumoTotalPrevisto. 
		*/
		void fazerPrevisaoConsumoTotal() {
			consumoTotalPrevisto = Previsor::preverConsumoTotal(hash, consumoProprioPrevisto);
		}

		/*!
			Método que realiza a sincronização entre as tomadas a cada 6 horas.
			\sa administrar()
		*/
		void iniciar() {
			Chronometer* cronPrincipal = new Chronometer();
			Data data = relogio->getData();
			
			long long tempoQuePassou = ((data.hora*60 + data.minuto) % (6*60)) * 60 * 1000000;
			long long timeTillNextWake;
			long long milisecsEm6Horas = 6*60*60*1000000.0;
			
			while (true) {
				
				timeTillNextWake = milisecsEm6Horas - tempoQuePassou;
				
				Alarm::delay(timeTillNextWake);
				
				cronPrincipal->reset();
				cronPrincipal->start();
				
				administrar();
				
				cronPrincipal->stop();
				tempoQuePassou = cronPrincipal->read();

			}
		}
		
		/*!
			Método que baseado no horário atual descobre a prioridade certa.
			\return Valor da prioridade da tomada no período atual do dia.
		*/
		int prioridadeAtual() {
			Prioridades prioridades = tomada->getPrioridades();
			unsigned int hour = relogio->getData().hora;
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
		void mantemConsumoDentroDoLimite() { 
			float diferencaConsumo, consumoCalculado = 0, consumoMesmaPioridade = consumoProprioPrevisto, menorConsumoMesmaPrioridade = 0;
			bool outrasComMesmaPrioridade = false; // Indica se há outras tomadas com a mesma prioridade
			
			for(auto iter = hash->begin(); iter != hash->end(); iter++) {
				// Se iter não é vazio: begin() retorna um objeto vazio no inicio por algum motivo
				if (iter != 0) {
					if(prioridadeAtual() > iter->object()->prioridade) { // Outras tomadas que têm prioridade abaixo da minha prioridade
						consumoCalculado += iter->object()->consumoPrevisto;
					} else if (prioridadeAtual() == iter->object()->prioridade) { // Outras tomadas com a mesma prioridade
						outrasComMesmaPrioridade = true;
						consumoMesmaPioridade += iter->object()->consumoPrevisto;
						if (consumoProprioPrevisto > iter->object()->consumoPrevisto) { // Tomadas cujo consumo é menor que o consumo desta tomada
							menorConsumoMesmaPrioridade += iter->object()->consumoPrevisto;
						}
					}
				}
			}
			
			float consumoRestante = maximoConsumoMensal - consumoMensal, sobraDeConsumo = 0;
			diferencaConsumo = consumoTotalPrevisto - consumoCalculado;
			if (diferencaConsumo <= consumoRestante) {
				//liga a tomada
				tomada->ligar();
			} else { // Tomada pode precisar se desligar ou dimmerizar(se possível)
				if (outrasComMesmaPrioridade) {
					// Se desligando as tomada de mesmo consumo completamente o consumo fica abaixo do necessário para o consumo máximo ser mantido
					if ((diferencaConsumo - consumoMesmaPioridade) < consumoRestante) {
						// Nesse caso, desligando apenas algumas tomadas de mesma prioridade que essa o consumo ainda pode ser mantido abaixo do limite
						// Aqui, as tomadas com consumo menor são as escolhidas para desligamento
						if ((diferencaConsumo - menorConsumoMesmaPrioridade)  <= consumoRestante) { // Se desligando as tomadas com mesma prioridade mas consumo menor é o suficiente para manter o limite de consumo
							// Essa tomada pode ser ligada
							tomada->ligar();
							
						} else { //Tomada é desligada ou dimmerizada
							if ((diferencaConsumo - menorConsumoMesmaPrioridade - consumoProprioPrevisto)  <= consumoRestante && tomada->getTipo() == 2) {
								// Se pode dimmerizar
								sobraDeConsumo = consumoRestante - (diferencaConsumo - menorConsumoMesmaPrioridade - consumoProprioPrevisto);
								static_cast<TomadaMulti*>(tomada)->dimmerizar(consumoProprioPrevisto, sobraDeConsumo);
							} else {
								tomada->desligar();
							}
						}
						
					} else {
						tomada->desligar();
					}
					
				} else { // Se não há outras tomadas com a mesma prioridade que essa		
					 // Se desligando a tomada completamente o consumo fica abaixo do necessário para o consumo máximo ser mantido e a tomada tem dimmer
					if ((diferencaConsumo - consumoProprioPrevisto) < consumoRestante && tomada->getTipo() == 2) {
						sobraDeConsumo = consumoRestante - (diferencaConsumo - consumoProprioPrevisto);
						static_cast<TomadaMulti*>(tomada)->dimmerizar(consumoProprioPrevisto, sobraDeConsumo);
					} else { // Se não tem dimmer
						tomada->desligar();
					}
				}	
			}		
		}
	
		/*!
			Método que calcula quantos 1/4 de dia faltam para o fim do mês. O valor é armazenado na variável global quantidade6Horas.
		*/
		void calculaQuantidadeQuartosDeDia() {
			quantidade6Horas = diasRestantes() * 4;
			int hora = relogio->getData().hora;
			// ajusta da quantidade para quando a tomada é criada depois do primeiro 1/4 do dia.
			int quartoDoDia = (int) hora / 6;
			quantidade6Horas = quantidade6Horas - quartoDoDia;		
		}
		
		/*!
			Método que calcula quantos dias restam para o mês acabar.
			\return Valor inteiro que indica quantos dias faltam para o fim do mês.
		*/
		int diasRestantes() {
			unsigned int diaAtual = relogio->getData().dia;
			int diasNoMes = relogio->getDiasNoMes();
			return (diasNoMes - diaAtual);		
		}
};

//----------------------------------------------------------------------------
//!  Método Main
/*!
	Método inicial do programa.
*/
int main() {
	
	Alarm::delay(2*1000000);

	cout << "Start" << endl << endl;
	
	Mensageiro* m = new Mensageiro();
	
	Tabela* h = new Tabela();;

	Hash_Element* e0;
	Hash_Element* e1;
	Hash_Element* e2;
	Dados* dados0 = new Dados();
	Dados* dados1 = new Dados();
	Dados* dados2 = new Dados();
	
	dados0->remetente = m->obterEnderecoNIC();
	dados0->ligada = true;
	dados0->consumoPrevisto = 100;
	dados0->ultimoConsumo = 200;
	dados0->prioridade = 0;
	
	dados1->remetente = m->obterEnderecoNIC();
	dados1->ligada = true;
	dados1->consumoPrevisto = 110;
	dados1->ultimoConsumo = 201;
	dados1->prioridade = 1;
	
	dados2->remetente = m->obterEnderecoNIC();
	dados2->ligada = true;
	dados2->consumoPrevisto = 120;
	dados2->ultimoConsumo = 202;
	dados2->prioridade = 2;
	
	cout << dados0->remetente << endl;
	cout << dados1->remetente << endl;
	cout << dados2->remetente << endl;
	
	h->insert(e0);
	h->insert(e1);
	h->insert(e2);
	
	for(auto iter = h->begin(); iter != h->end(); iter++) {
		if (iter != 0) {
			cout << iter->object()->prioridade << endl;
		}
	}
	
	//float previsao = Previsor::preverConsumoTotal(h, 4, 90); a chamada do método foi mudada
	//cout << previsao << endl;

	
	while (true);
	
	//t.setPrioridades
	//Relogio r;
	//Gerente* g = new Gerente(t);
	//g.iniciar();
};
