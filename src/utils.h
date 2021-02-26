#define QUANT_MAX_REQS 100
#define QUANT_REQS_PLANO 20
#define NOME 50               //tamanho do nome dos requisitos
#define P1_WEIGHT 10000
#define P2_WEIGHT 1000
#define P3_WEIGHT 100
#define P4_WEIGHT 10
#define P5_WEIGHT 1

//Nodo pra lista (clusters)
typedef struct nd{
  char id[256];
  int size;
  char dependencies[256];
  struct nd *next, *prev;
} node;

node **clusters;

void include(char* id, int priority, int size, char* dependencies);
void imprimeClusters(int num_priorities);
void le_xml();
void gv_input(int num_priorities);
void create_domain_file(int num_teams, int num_priorities, int balance);
void create_problem_file(int num_teams, int capacity, int num_priorities, int balance);
void cria_arquivo_gv(FILE *graph_file, char result[][QUANT_REQS_PLANO][NOME], int count[], int releases, int num_teams);
void le_parametros(int argc, char const *argv[], int *tamanho_total, int *planos_paralelos, int *num_priorities, int *balance);

//  EXPAT
#define BUFFSIZE 8192
char Buff[BUFFSIZE];
int Depth;
//start chama a função include que insere os requisitos nos clusters
void XMLCALL start(void *data, const XML_Char *el, const XML_Char **attr);
void XMLCALL end(void *data, const XML_Char *el);
