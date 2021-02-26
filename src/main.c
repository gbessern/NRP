#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include "utils.h"
#include <string.h>
#include <math.h>

int main(int argc, char const *argv[]) {
        /*Variaveis:
           command: linha de comando usado para chamar o metric-FF atraves da funcao system()
           step: proximo requisito a ser adicionado no plano
           reqs_verificados: lista de requisitos já adicionados em algum plano
           plan_file: arquivo aberto (resultado do metric-ff)
           req: usado para percorrer as listas de requisitos (variavel global clusters)
         */

        char command[256], str[256], *step;
        int num_teams=1, num_priorities=5, capacity=0, team, releases, balance=0;
        FILE *plan_file, *graph_file;
        // node *aux;
        
        system("mkdir -p files");
        graph_file = fopen("files/graph.dot","w");
        fprintf(graph_file,"digraph NEXT_RELEASE_PROBLEM {\n");
        fprintf(graph_file,"  node [style=filled];\n");
        fprintf(graph_file,"  label=\"Plano\";\n");
        fprintf(graph_file,"  labelloc=\"t\";\n");
          
        le_parametros(argc, argv, &capacity, &num_teams, &num_priorities, &balance);

        //inicia listas de requisitos (uma lista para cada prioridade)
        clusters = malloc(num_priorities*sizeof(node*));
        for(int i=0; i<num_priorities; i++)
                clusters[i] = NULL;

        char result[num_teams][QUANT_REQS_PLANO][NOME];
        int count[num_teams];

        le_xml();
        //imprimeClusters(num_priorities);

        gv_input(num_priorities);

        for( int i=0; i<num_teams; i++)
          count[i] = 0;

        printf("Creating problem file...\n");
        create_problem_file(num_teams, capacity, num_priorities, balance);
        
        printf("Creating domain file...\n");
        create_domain_file(num_teams, num_priorities, balance);
        //sprintf(command, "cp src/domain.pddl files/");

        printf("Calling Metric-FF\n");
        //chama o metric-ff e salva o resultado em files
        sprintf(command, "./src/Metric-FF-v2.1/ff -s 0 -p ./files/ -o domain.pddl -f problem.pddl > ./files/plan_file");
        system(command);
        printf("Reading created plan\n");
        //abre plano gerado
        plan_file = fopen("files/plan_file", "r");

        //procura a parte do arquivo que traz os passos do planos. Talvez de para usar strtok() no lugar desse for.
        for( fgets(str, 256, plan_file); strcmp(str, "ff: found legal plan as follows\n"); fgets(str, 256, plan_file) )
          if( feof(plan_file) ) {
          printf("plano nao gerado!\n");
          exit(1);
        }

        //le uma linha, separa a parte do nome do step. Se esse nome for de um requisito valido, o requisito eh adicionado ao
        //planos atual, aos requisitos processados e a capcidade restante eh reduziada
        releases = 0;
        while( strcmp(fgets(str, 256, plan_file), "\n") ) {
        step = strtok( str, ":" );
        step = strtok( NULL, ": \n");
          if( strcmp(step, "INCREASE_PRIORITY") && strncmp(step, "RESET_CAPACITY", 14) && strcmp(step, "CHANGE_TEAM") ){
            if( !strcmp(step, "NEW_SPRINT") ){
              cria_arquivo_gv(graph_file, result, count, releases++, num_teams);
              for( int i=0; i<num_teams; i++)
                count[i] = 0;
            }
            else{
                step = strtok( NULL, ": \n");
                strcpy(str, step);
                step = strtok( NULL, ": \n");
                team = step[4] - '0';
                team--;
                strcpy(result[team][count[team]], str);
                count[team]++;
            }
          }
        }
        cria_arquivo_gv(graph_file, result, count, releases++, num_teams);
        

        fseek(graph_file,0,SEEK_END);
        // for (int i = 0; i < num_priorities; i++){
        //   aux = clusters[i];
        //   while( aux != NULL){
        //     fprintf(graph_file, " %s [color=%d];\n", aux->id, (int)floor((10/num_priorities)*(i+1)));
        //     aux = aux->next;
        //   }
        // }
        fprintf(graph_file,"}\n");
        fclose(graph_file);
        printf("Calling Graphviz\n");
        system("dot -Tpdf files/graph.dot -o plano.pdf");
        //system("rm -r graph.dot planos pddls");

        return 0;
}
