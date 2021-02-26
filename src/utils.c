#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <expat.h>
#include "utils.h"
#include <math.h>

//Adiciona um requisito no cluster de sua prioridade
void include(char *id, int priority, int size, char *dependencies)
{
	if (clusters[priority - 1] == NULL)
	{
		clusters[priority - 1] = (node*) malloc(sizeof(node));
		strcpy(clusters[priority - 1]->id, id);
		clusters[priority - 1]->size = size;
		strcpy(clusters[priority - 1]->dependencies, dependencies);
		clusters[priority - 1]->next = NULL;
		clusters[priority - 1]->prev = NULL;
	}
	else
	{
		node *n = clusters[priority - 1];
		for (; n->next != NULL; n = n->next);
		n->next = (node*) malloc(sizeof(node));
		n->next->prev = n;
		n = n->next;
		n->next = NULL;
		strcpy(n->id, id);
		n->size = size;
		strcpy(n->dependencies, dependencies);
	}
}

void imprimeClusters(int num_priorities)
{
	node * aux;

	for (int i = 1; i <= num_priorities; i++)
	{
		printf("CLUSTER DE PRIORIDADE %d\n", i);
		aux = clusters[i - 1];
		while (aux != NULL)
		{
			printf("id: %s  size: %d  dependencies: %s\n", aux->id, aux->size, aux->dependencies);
			aux = aux->next;
		}

		printf("\n");
	}
}

void create_problem_file(int num_teams, int capacity, int num_priorities, int balance)
{
	FILE * problem;
	node * aux;
	char *dep, dependencies[256];
	unsigned long long int sum=0;


	system("mkdir -p files");
	problem = fopen("files/problem.pddl", "w");
	if (problem == NULL)
	{
		printf("Erro ao criar arquivo de problema!");
		exit(1);
	}

	fprintf(problem, "(define (problem nrl)\n");
	fprintf(problem, "    (:domain nrl)\n");

	//OBJECTS
	fprintf(problem, "    (:objects\n        ");
	for (int i = 0; i < num_priorities; i++)
	{
		aux = clusters[i];
		while (aux != NULL)
		{
			fprintf(problem, "%s ", aux->id);
			aux = aux->next;
		}
	}
	fprintf(problem, "- requirement\n        ");

	for(int i=num_teams; i>=1; i--)
		fprintf(problem, "team%d ", i);
	fprintf(problem, "- team\n        ");
	for(int i=0; i<=num_priorities; i++)
		fprintf(problem, "P%d ", i);
	fprintf(problem, "- priorities\n");	
	fprintf(problem, "    )\n\n");

	//INITIAL STATE
	fprintf(problem, "    (:init\n");
	fprintf(problem, "        (= (sum) 0)\n\n");
	fprintf(problem, "        (= (max_capacity) %d)\n", capacity);
	for(int t=1; t<=num_teams; t++)
		fprintf(problem, "        (= (capacity team%d) %d)\n", t, capacity);

	fprintf(problem, "\n        (priority P1)\n");
	
	if( balance )
		fprintf(problem, "\n        (team team1)\n\n");


	//DEPENDENCIES
	for (int i = 0; i < num_priorities; i++)
	{
		aux = clusters[i];
		while (aux != NULL)
		{
			strcpy(dependencies, aux->dependencies);
			dep = strtok(dependencies, ",");
			for (; dep; dep = strtok(NULL, ","))
			fprintf(problem, "        (dependent %s %s)\n", aux->id, dep);
			aux = aux->next;
		}
	}
	fprintf(problem, "\n");

	//SIZES
	for (int i = 0; i < num_priorities; i++)
	{
		aux = clusters[i];
		while (aux != NULL)
		{
			fprintf(problem, "        (= (size %s) %d)\n", aux->id, aux->size);
			aux = aux->next;
		}
	}
	fprintf(problem, "\n");

	//num_priorities
	for (int i = 0; i <num_priorities; i++)
	{
		aux = clusters[i];
		while (aux != NULL)
		{
			sum += (int)pow(10, num_priorities-i-1);
			fprintf(problem, "        (r_priority %s P%d)\n", aux->id, i+1);
			aux = aux->next;
		}
	}
	fprintf(problem, "\n");

	for (int i = num_priorities; i>0; i--){
			fprintf(problem, "        (= (weight P%d) %llu)\n", i, (unsigned long long int)pow(10,num_priorities-i));

	}
	fprintf(problem, "\n    )");

	fprintf(problem, "    (:goal\n        (and (= (sum) %llu))\n    )\n)", sum);
	fclose(problem);
}

void create_domain_file(int num_teams, int num_priorities, int balance)
{
	FILE * domain;

	//cria aquivo de dominio
	//system("mkdir -p files");
	domain = fopen("files/domain.pddl", "w");
	if (domain == NULL)
	{
		printf("Erro ao criar arquivo de dominio!");
		exit(1);
	}

	fprintf(domain, "(define (domain nrl)\n");
	fprintf(domain, "	(:requirements :typing :fluents :negative-preconditions :disjunctive-preconditions)\n");
	fprintf(domain, "	(:types requirement team priorities)\n\n");

	//PREDICADOS
	fprintf(domain, "  (:predicates\n");
	fprintf(domain, "		(done_all ?r - requirement)\n");
	fprintf(domain, "		(done_team ?r - requirement ?t - team)\n");
	fprintf(domain, "		(dependent ?r ?req - requirement)\n");
	fprintf(domain, "		(r_priority ?r - requirement ?p - priorities)\n");
	fprintf(domain, "		(priority ?p - priorities)\n");
	if( balance )
		fprintf(domain, "		(team ?t - team)\n");
	fprintf(domain, "	)\n\n");

	//FUNÇÕES
	fprintf(domain, "	(:functions\n");
	fprintf(domain, "		(max_capacity)\n");
	fprintf(domain, "		(capacity ?t - team)\n");
	fprintf(domain, "		(size ?r - requirement)\n");
	fprintf(domain, "		(sum)\n");
	fprintf(domain, "		(weight ?p - priorities)\n");
	fprintf(domain, "	)\n\n");

	//AÇÃO DEV_REQ
	fprintf(domain, "	(:action DEV_REQ\n");
	fprintf(domain, "		:parameters (?r - requirement ?t - team ?p - priorities)\n");
	fprintf(domain, "		:precondition (and\n");
	fprintf(domain, "			(priority ?p)\n");
	if( balance )
		fprintf(domain, "			(team ?t)\n");
	fprintf(domain, "			(r_priority ?r ?p)\n");
	fprintf(domain, "			(>= (capacity ?t) (size ?r))\n");
	fprintf(domain, "			(forall (?team - team)\n");
	fprintf(domain, "				(not (done_team ?r ?team))\n");
	fprintf(domain, "			)\n");
	fprintf(domain, "			(forall (?req  - requirement)\n");
	fprintf(domain, "				(imply (dependent ?r ?req) (or (done_team ?req ?t) (done_all ?req)))\n");
	fprintf(domain, "			)\n");
	fprintf(domain, "		)\n");
	fprintf(domain, "		:effect (and\n");
	fprintf(domain, "			(done_team ?r ?t)\n");
	fprintf(domain, "			(decrease (capacity ?t) (size ?r))\n");
	fprintf(domain, "			(increase (sum) (weight ?p))\n");
	fprintf(domain, "			(not (priority ?p))\n");
	fprintf(domain, "			(priority P1)\n");
	if( balance )
		for(int i=1; i<=num_teams; i++){
			if(i != num_teams)
				fprintf(domain, "			(when (and (team team%d)) (and (not (team team%d)) (team team%d)))\n", i, i, i+1);
			else
				fprintf(domain, "			(when (and (team team%d)) (and (not (team team%d)) (team team1)))\n", i, i);
		}
	fprintf(domain, "		)\n");
	fprintf(domain, "	)\n");

	// AÇÃO SPRINT
	fprintf(domain, "	(:action NEW_SPRINT\n");
	fprintf(domain, "		:parameters ()\n");
	fprintf(domain, "		:precondition (and \n");
	fprintf(domain, "			(forall (?t - team)\n");
	fprintf(domain, "				(= (capacity ?t) 0)\n");
	fprintf(domain, "			)\n");
	fprintf(domain, "		)\n");
	fprintf(domain, "		:effect (and \n");
	fprintf(domain, "			(forall (?r - requirement ?t - team)\n");
	fprintf(domain, "				(when (and (done_team ?r ?t)) (and (done_all ?r)))\n");
	fprintf(domain, "			)\n");
	fprintf(domain, "			(forall (?t - team)\n");
	fprintf(domain, "				(and (assign (capacity ?t) (max_capacity))\n");
	if( balance )
		fprintf(domain, "					(not (team ?t))\n");
	fprintf(domain, "				)\n");
	fprintf(domain, "			)\n");
	if( balance ){
		fprintf(domain, "			(team team1)\n");
		fprintf(domain, "			(forall (?p - priorities)\n");
		fprintf(domain, "				(not (priority ?p))\n");
		fprintf(domain, "			)\n");
	}
	fprintf(domain, "			(priority P1)\n");
	fprintf(domain, "		)\n");
	fprintf(domain, "	)\n");

	//AÇÃO INCREASE_PRIORITY
	fprintf(domain, "	(:action INCREASE_PRIORITY\n");
	fprintf(domain, "		:parameters ()\n");
	fprintf(domain, "		:precondition (and )\n");
	fprintf(domain, "		:effect (and \n");
	for(int i=1; i<=num_priorities; i++){
		if(i == num_priorities){
			fprintf(domain, "			(when (and (priority P%d)) (and (not (priority P%d)) (priority P0)))\n", i, i);
		}
		else{
			fprintf(domain, "			(when (and (priority P%d)) (and (not (priority P%d)) (priority P%d)))\n", i, i, i+1);
		}
	}
	fprintf(domain, "		)\n");
	fprintf(domain, "	)\n");

	//AÇÃO CHANGE_TEAM
	if( balance ){
		fprintf(domain, "	(:action CHANGE_TEAM\n");
		fprintf(domain, "		:parameters ()\n");
		fprintf(domain, "		:precondition (and )\n");
		fprintf(domain, "		:effect (and \n");
		for(int i=1; i<=num_teams; i++){
			if(i != num_teams)
				fprintf(domain, "			(when (and (team team%d)) (and (not (team team%d)) (team team%d)))\n", i, i, i+1);
			else
				fprintf(domain, "			(when (and (team team%d)) (and (not (team team%d)) (team team1)))\n", i, i);
		}
		fprintf(domain, "			(forall (?p - priorities)\n");
		fprintf(domain, "				(not (priority ?p))\n");
		fprintf(domain, "			)\n");
		fprintf(domain, "			(priority P1)\n");
		fprintf(domain, "		)\n");
		fprintf(domain, "	)\n");
	}

	//AÇÃO RESET_CAPACITY
	fprintf(domain, "	(:action RESET_CAPACITY\n");
	fprintf(domain, "		:parameters (?t - team)\n");
	fprintf(domain, "		:precondition (and (priority P0)\n");
	if( balance )
		fprintf(domain, "			(team ?t)\n");
	fprintf(domain, "		)\n");
	fprintf(domain, "		:effect (and (assign (capacity ?t) 0)\n");
	if( balance )
		for(int i=1; i<=num_teams; i++){
			if(i != num_teams)
				fprintf(domain, "			(when (and (team team%d)) (and (not (team team%d)) (team team%d)))\n", i, i, i+1);
			else
				fprintf(domain, "			(when (and (team team%d)) (and (not (team team%d)) (team team1)))\n", i, i);
		}
	fprintf(domain, "			(not (priority P0))\n");
	fprintf(domain, "			(priority P1)\n");
	fprintf(domain, "		)\n");
	fprintf(domain, "	)\n");

	fprintf(domain, ")\n");
	fclose(domain);
}

void le_xml()
{
	//aloca parser (Expat)
	XML_Parser p = XML_ParserCreate(NULL);
	if (!p)
	{
		fprintf(stderr, "Couldn't allocate memory for parser\n");
		exit(-1);
	}

	//inicia parser. start e end sao definidas em utils.c
	XML_SetElementHandler(p, start, end);

	//leitura do XML de entrada
	for (;;)
	{
		int done;
		int len;

		len = (int) fread(Buff, 1, BUFFSIZE, stdin);
		if (ferror(stdin))
		{
			fprintf(stderr, "Read error\n");
			exit(-1);
		}

		done = feof(stdin);

		if (XML_Parse(p, Buff, len, done) == XML_STATUS_ERROR)
		{
			fprintf(stderr,
				"Parse error at line %lu:\n%s\n",
				XML_GetCurrentLineNumber(p),
				XML_ErrorString(XML_GetErrorCode(p)));
			exit(-1);
		}

		if (done)
			break;
	}

	XML_ParserFree(p);
}

void XMLCALL start(void *data, const XML_Char *el, const XML_Char **attr)
{
	(void) data;
	char id[256], dependencies[256], atribute[256];
	int priority, size;

	strcpy(id, el);

	for (int i = 0; attr[i]; i += 2)
	{
		strcpy(atribute, attr[i]);
		for (int j = 0; atribute[j]; j++)
			atribute[j] = tolower(atribute[j]);

		if (strcmp(atribute, "prioridade") == 0 || strcmp(atribute, "prioridades") == 0)
			priority = atoi(attr[i + 1]);
		else if (strcmp(atribute, "tamanho") == 0 || strcmp(atribute, "tamanhos") == 0)
			size = atoi(attr[i + 1]);
		else if (strcmp(atribute, "dependencia") == 0 || strcmp(atribute, "dependencias") == 0)
			strcpy(dependencies, attr[i + 1]);
	}

	if (Depth > 0)
		include(id, priority, size, dependencies);

	Depth++;
}

void XMLCALL end(void *data, const XML_Char *el)
{
	(void) data;
	(void) el;

	Depth--;
}

void cria_arquivo_gv(FILE *graph_file, char result[][QUANT_REQS_PLANO][NOME], int count[], int releases, int num_teams)
{
	if (graph_file == NULL)
	{
		printf("Erro ao criar arquivo .dot!");
		exit(1);
	}

	fseek(graph_file, 0, SEEK_END);

	fprintf(graph_file, "  subgraph cluster%d {\n    ", releases);
	for (int team = 0; team < num_teams; team++){
		if(count[team] > 0){
			fprintf(graph_file, "	T_%d%d -> ", team+1, releases);
			for (int r = 0; r < count[team]-1; r++){
				fprintf(graph_file, "%s -> ", result[team][r]);
			}
			fprintf(graph_file, "%s;\n", result[team][count[team]-1]);
		}
	}
	fprintf(graph_file, "    label = \"Versão #%d\";\n", releases+1);
	for (int team = 0; team < num_teams; team++)
		fprintf(graph_file, "		T_%d%d [label=\"Time %d\" shape=box style=\"solid\"];\n", team+1, releases, team+1);
	
	if( num_teams > 1){
		for (int team = 0; team < num_teams-1; team++)
			fprintf(graph_file, "T_%d%d -> ", team+1, releases);
		fprintf(graph_file, "T_%d%d [style=invis]; ", num_teams, releases);
	
		fprintf(graph_file, "{rank=same; ");
		for (int team = 0; team < num_teams; team++)
			fprintf(graph_file, "T_%d%d; ", team+1, releases);
		fprintf(graph_file, "}\n");
	}
	fprintf(graph_file, " }\n");
}

void gv_input(int num_priorities){
	FILE *graph_file;
	node *aux;
	char *dep, dependencies[256];

	graph_file = fopen("files/input.dot","w");
    fprintf(graph_file,"digraph NEXT_RELEASE_PROBLEM_INPUT {\n");
    fprintf(graph_file,"node [colorscheme=spectral10, style=filled];\n");
    fprintf(graph_file,"label=\"Entrada\";\n");
    fprintf(graph_file,"labelloc=\"t\";\n");

	
	if (graph_file == NULL)
	{
		printf("Erro ao criar arquivo .dot de entrada!");
		exit(1);
	}

	for (int i = 0; i < num_priorities; i++)
	{
		aux = clusters[i];
		if(aux != NULL)
		while (aux != NULL)
		{
			// fprintf(graph_file, "	%s [color=%d];\n", aux->id, (int)floor((10/num_priorities)*(i+1)));
			fprintf(graph_file, "	%s [label=\"%s\\nT=%d e P=%d\"];\n", aux->id, aux->id, aux->size, i+1);
			strcpy(dependencies, aux->dependencies);
			dep = strtok(dependencies, ",");
			for (; dep; dep = strtok(NULL, ","))
			fprintf(graph_file, "	%s -> %s;\n", aux->id, dep);
			aux = aux->next;
		}
	}

	fprintf(graph_file,"}\n");
    fclose(graph_file);
    printf("Calling Graphviz for input\n");
    system("dot -Tpdf files/input.dot -o entrada.pdf");
}



void le_parametros(int argc, char
	const *argv[], int *tamanho_total, int *planos_paralelos, int *num_priorities, int *balance)
{
	*planos_paralelos = 1;
	if (argc == 1)
	{
		printf("Necessario passar tamanho maximo de cada plano);\n");
		printf("Uso: ./nrl -t TAMANHO_MAXIMO [-e QUANTIDADE_PLANOS_PARALELOS][-p QUANTIDADE_PRIORIDADES][-b BALANCEAMENTO] < entrada.xml\n\n");
		printf("Balanceamentos:\n");
		printf("	0 - maximiza numero de requisitos entregue aos times, buscando deixar livre times quando possivel\n");
		printf("	1 - Procura igualar numerio de requisitos entregues a cada time\n");

		exit(1);
	}
	else
	{
		for (int i = 1; i < argc; i += 2){
			if (!strcmp(argv[i], "-t"))
				*tamanho_total = atoi(argv[i + 1]);
			else if (!strcmp(argv[i], "-e"))
				*planos_paralelos = atoi(argv[i + 1]);
			else if (!strcmp(argv[i], "-p"))
				*num_priorities = atoi(argv[i + 1]);
			else if (!strcmp(argv[i], "-b"))
				*balance = atoi(argv[i + 1]);
			else
			{
				printf("parametro invalido: %s\n", argv[i]);
				exit(1);
			}
		}
	}

	if (*tamanho_total == 0)
	{
		printf("Necessario passar tamanho maximo de cada plano);\n");
		printf("Uso: ./nrl -t TAMANHO_MAXIMO [-e QUANTIDADE_PLANOS_PARALELOS][-p QUANTIDADE_PRIORIDADES][-b BALANCEAMENTO] < entrada.xml\n\n");
		printf("Balanceamentos:\n");
		printf("	0 - maximiza numero de requisitos entregue aos times, buscando deixar livre times quando possivel\n");
		printf("	1 - Procura igualar numerio de requisitos entregues a cada time\n");

		exit(1);
	}
}