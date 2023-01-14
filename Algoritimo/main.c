#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <io.h>
#include <ctype.h>
#include <math.h>
#include <conio.h>

//TAD para as StopWords
typedef struct stopWords
{
    char p[50];

} sw;

typedef struct termos
{
    char termo[50];

} ListaTermos;


//TAD para os tokens de cada Documento
typedef struct tokens
{
    char token[50];
} tokensDoc;


typedef struct docs
{
    int id;
    char documento[1000];
    tokensDoc *token;
    int qtdTokens;
    struct docs *prox;
    struct docs *ant;

} docs;

typedef struct Indice
{
    char termo[50];
    int *docs;
    int qtdT;

} indice;

typedef struct tf_docs
{
    int idDoc;
    int quant;
    float tfIdf;

} tf_docs;

typedef struct tf_idf
{
    char termo[50];
    tf_docs *docs;

} tf_idf;

//Listas de pontuações e StopWords a serem removidas

sw *palavrasRemover;

typedef void TFuncOnFindDir(char *);
docs *ListaDocumentos;
indice *ind;
char diretorio[80];
char idDoc = 0;

//Inicializa a Lista de Documentos em NULL
void criaLista()
{
    ListaDocumentos = NULL;
}

void criaIndice()
{
    ind = NULL;
}

//Aloca memoria para a quantidade de Stop Words necessarias
void criarStopWords(int quant)
{
    palavrasRemover = (sw *)malloc(quant * sizeof(sw));
}

//Retorna a quantidade de stop words, para alocação de memoria
int quantidadeStopWords()
{

    FILE *f = fopen("SW.txt","r");
    char *result;
    int quant = 0;
    char Linha[100];
    char result2[1000];

    if(f == NULL)
    {
        printf("\nErro ao abrir arquivo\n");
        return 0;
    }

    while (!feof(f))
    {
        result = fgets(Linha, 100, f);
        if (result)
        {
            quant++;
        }
    }

    fclose(f);

    return quant;

}

int quantidadePontuacao()
{

    FILE *f = fopen("pontuacao.txt","r");
    char *result;
    int quant = 0;
    char Linha[100];
    char result2[1000];

    if(f == NULL)
    {
        printf("\nErro ao abrir arquivo\n");
        return 0;
    }

    while (!feof(f))
    {
        result = fgets(Linha, 100, f);
        if (result)
        {
            quant++;
        }
    }

    fclose(f);

    return quant;

}

//Busca pelos arquivos em um certo diretório, usando uma mascara (No nosso caso, *.txt)
int ListaDiretorio(char *dir, char *mask, TFuncOnFindDir *func)
{
    int done;
    struct _finddata_t find;
    char fname[80];

    strcpy(fname,dir);

    if(fname[strlen(fname) - 1]!='\\')
    {
        strcat(fname,"\\");
    }

    strcat(fname,mask);
    done = _findfirst(fname,&find);

    if(done < 0)
    {
        return 1;
    }

    do
    {
        if(find.attrib & _A_ARCH)
        {
            if(func)
            {
                //Chama a função OnFind passando o nome do arquivo
                func(find.name);
            }
        }

    }
    while(!_findnext(done,&find));
    _findclose(done);

    return 0;
}

//Funçao para receber os nomes dos arquivos e abri-los
void OnFind(char *fname)
{
    //Faz as modificações necessarias na string para obter o caminho absoluto do arquivo
    char nomeArq[200];
    strcpy(nomeArq,diretorio);
    strcat(nomeArq,"/");
    strcat(nomeArq,fname);

    FILE *f;
    f = fopen(nomeArq,"r");

    if(f == NULL)
    {
        printf("\nErro ao abrir arquivo %s\n",fname);
        return;
    }

    //Chamamos a função para ler o conteudo de nossa lista
    lerDocumento(f);
    fclose(f);

    //Zeramos o nome do arquivo
    for(int i=0; nomeArq[i]!='\0'; i++)
    {
        nomeArq[i] = '\0';
    }

}

//Lemos o conteudo do documento concatenando as linhas
//Ao final do arquivo, chamamos a função de salvar o documento em nossa lista
void lerDocumento(FILE *f)
{
    char Linha[100];
    char *result;
    char result2[1000];
    int flag = 0;

    while (!feof(f))
    {
        result = fgets(Linha, 100, f);
        if (result)
        {
            strcat(result2,strlwr(result));
        }
    }

    salvarDocumento(result2);
}

//Verificamos se o termo a ser inserido é uma StopWord, caso não podemos inserir
int verificaStopWords(char *text, int tam)
{
    int flag = 0;
    char auxText[50];

    for(int i=0; auxText[i]!='\0'; i++)
    {
        auxText[i] = '\0';
    }

    strcpy(auxText,text);
    auxText[tam] = '\n';

    for(int i=0; i<258; i++)
    {
        if(strcmp(auxText,palavrasRemover[i].p)==0)
        {
            flag = 1;
            return 1;
        }
    }

    return 0;
}

//Verificamos se o token ja existe, caso sim, ele é desconsiderado
int tokenJaExiste(char *text, docs *doc, int tam)
{

    for(int i=0; i<tam; i++)
    {
        if(strcmp(doc->token[i].token,text)==0)
        {
            return 1;
        }
    }

    return 0;
}

//Inserimos o documento em nossa Lista
void salvarDocumento(char *text)
{
    docs *doc = (docs *)malloc(sizeof(docs));

    char aux[50];
    int cont = 0;
    int contToken = 0,contAux = 0;
    int po,ex;
    int qtdAposLimpeza = 0;

    if(doc == NULL)
    {
        printf("\nErro na alocacao\n");
        return;
    }

    //Verificamos quantos termos temos no documento
    //Incluindo repetições e stop words
    for(int i=0; text[i]!='\0'; i++)
    {
        if(text[i] == ' ')
        {
            cont++;
        }
    }

    //Zeramos a string auxiliar para ser usada na primeira vez
    for(int k=0; aux[k]!='\0'; k++)
    {
        aux[k] = '\0';
    }

    //Para cada documento, temos uma lista de tokens, sendo assim
    //Alocamos cont posições para estes termos
    doc->token = (tokensDoc *)malloc((cont+1) * sizeof(tokensDoc));


    //Percorremos todo o documento
    for(int i=0; text[i]!='\0'; i++)
    {
        //Em caso de espaço, podemos concluir que chegamos ao final de uma palavra, e agora verificaremos se podemos inserir
        //ou se ela ja existe ou é uma StopWord
        if(text[i] == ' ')
        {
            po = verificaStopWords(aux,contAux);
            ex = tokenJaExiste(aux,doc,qtdAposLimpeza);

            //Caso nao seja StopWord nem repetição
            if(po==0&&ex==0)
            {
                strcpy(doc->token[contToken++].token,aux);
                qtdAposLimpeza++;
            }

            //Zeramos a string auxiliar
            for(int k=0; aux[k]!='\0'; k++)
            {
                aux[k] = '\0';
            }
            contAux = 0;
        }
        else if(text[i]!='!'&&text[i]!='"'&&text[i]!='#'&&text[i]!='$'&&text[i]!='%'&&text[i]!='&'&&text[i]!="'"&&
                text[i]!='('&&text[i]!=')'&&text[i]!='*'&&text[i]!='+'&&text[i]!=','&&text[i]!='-'&&text[i]!='.'&&
                text[i]!='/'&&text[i]!=':'&&text[i]!=';'&&text[i]!='<'&&text[i]!='='&&text[i]!='>'&&text[i]!='?'&&
                text[i]!='@'&&text[i]!='['&&text[i]!=']'&&text[i]!='^'&&text[i]!='_'&&text[i]!='`'&&
                text[i]!='{'&&text[i]!='|'&&text[i]!='}'&&text[i]!='~'&&text[i]!='\\')
        {

            if(text[i] != '\n')
            {
                aux[contAux++] = text[i];
            }
        }

    }
    aux[contAux] = '\0';

    //Verificamos mais uma vez, agora para a ultima palavra do documento, pois o '\0'
    //nao entra no for

    po = verificaStopWords(aux,contAux);
    ex = tokenJaExiste(aux,doc,qtdAposLimpeza);

    if(po==0&&ex==0)
    {
        strcpy(doc->token[contToken++].token,aux);
        qtdAposLimpeza++;
    }

    //Inserção dos valores do TAD
    doc->qtdTokens = qtdAposLimpeza;
    doc->ant = NULL;
    doc->prox = NULL;
    doc->id = idDoc++;
    strcpy(doc->documento,text);

    //Inserção do primeiro elemento
    if(ListaDocumentos == NULL)
    {
        ListaDocumentos = doc;
    }
    else
    {
        //Inserção do segundo elemento em diante
        docs *aux = ListaDocumentos;

        while(aux->prox!=NULL)
        {
            aux = aux->prox;
        }

        aux->prox = doc;
        doc->ant = aux;
    }
}

//Fazemos a inserção de nossas StopWords no TAD especifico
void inserirStopWord()
{
    FILE *f = fopen("SW.txt","r");

    char Linha[100];
    char *result;
    int i=0;

    if(f==NULL)
    {
        printf("\nErro nas StopWords\n");
        return;
    }

    while (!feof(f))
    {
        result = fgets(Linha, 100, f);
        if (result)
        {
            //Adicionamos em um ponteiro alocado dinamicamente
            strcpy(palavrasRemover[i++].p,result);
        }
    }

    fclose(f);
}

//Listamos os documentos e suas caracteristicas
void listar()
{
    docs *d = ListaDocumentos;

    while(d!=NULL)
    {
        printf("\nDOCUMENTO %d: \n\n",d->id +1);
        printf("Id documento: %d\n",d->id);
        printf("Quantidade de tokens: %d\n",d->qtdTokens);
        printf("Texto do documento: %s",d->documento);
        printf("Tokens: ");
        for(int i=0; i<d->qtdTokens; i++)
        {
            printf("%s  ",d->token[i].token);
        }
        printf("\n-----------------------------------------------------------------------------------");

        printf("\n");
        d = d->prox;
    }

    printf("\n");
    system("pause");
}

//Buscamos um termo especifico, e printamo em quais documentos ele aparece
void buscarTermo(char *termo)
{

    docs *doc = ListaDocumentos;
    int flag = 0;

    while(doc!=NULL)
    {
        for(int i=0; i<doc->qtdTokens; i++)
        {
            if(strcmp(termo,doc->token[i].token)==0)
            {
                printf("\nAchei no documento id = %d\n",doc->id);
                flag = 1;
                system("pause");
            }
        }
        doc = doc->prox;
    }

    if(flag == 0)
    {
        printf("\nNao encotramos nenhum documento correspondente ao termo buscado\n\n");
        system("pause");
    }

}
int quantidadeTermos()
{
    int qtd = 0;
    docs *aux;
    aux = ListaDocumentos;

    while(aux!=NULL)
    {
        qtd = qtd + aux->qtdTokens;
        aux = aux->prox;
    }

    return qtd;

}


int termoExiste(ListaTermos *listaTermos,int qtd, char *termo)
{
    for(int i=0; i<qtd; i++)
    {
        if(strcmp(termo,listaTermos[i].termo)==0)
        {
            return 1;
        }
    }

    return 0;
}

int insereTermo(ListaTermos *listaTermos,int qtd)
{
    docs *aux,*aux2;
    aux = ListaDocumentos;
    aux2 = ListaDocumentos;
    int flag = 1;

    int k = 0;

    while(aux2!=NULL)
    {
        for(int i=0; i<aux2->qtdTokens; i++)
        {
            flag = termoExiste(listaTermos,qtd,aux2->token[i].token);

            if(flag == 0)
            {
                strcpy(listaTermos[k++].termo,aux2->token[i].token);
            }
        }
        aux2 = aux2->prox;
    }

    return k;
}

void calculaIndice(ListaTermos *listaTermos, indice *ind, int qtd)
{
    docs *aux,*aux2;
    aux = ListaDocumentos;
    aux2 = ListaDocumentos;
    int k = 0;
    int qtdTermo = 0;
    int cont = 0,l = 0;
    int cont2 = 0;

    while(cont < qtd)
    {
        qtdTermo = 0;
        cont2 = 0;
        l = 0;

        while(aux)
        {
            for(int i=0; i<aux->qtdTokens; i++)
            {
                if(strcmp(aux->token[i].token,listaTermos[k].termo)==0)
                {
                    qtdTermo++;
                }
            }

            aux = aux->prox;
        }

        ind[k].docs = (int *)malloc(qtdTermo * sizeof(int));
        ind[k].qtdT = qtdTermo;
        strcpy(ind[k].termo,listaTermos[k].termo);

        while(aux2)
        {

            for(int i=0; i<aux2->qtdTokens; i++)
            {
                if(strcmp(aux2->token[i].token,listaTermos[k].termo)==0)
                {
                    ind[k].docs[l++] = aux2->id;
                }
            }

            aux2 = aux2->prox;
        }

        k++;
        aux = ListaDocumentos;
        aux2 = ListaDocumentos;
        cont++;

    }

}

int quantDocumentos()
{

    docs *aux = ListaDocumentos;
    int i =0;

    while(aux)
    {
        i++;
        aux = aux->prox;
    }

    return i;
}

void teste(ListaTermos *listaTermos)
{
    printf("\n%s\n",listaTermos[0].termo);
}


int menu()
{
    int op;

    system("cls");
    printf("------------------TRABALHO ORI-----------------\n\n");
    printf("  -> 1 - Visualizar Documentos Abertos\n");
    printf("  -> 2 - Buscar documentos que possuam um certo termo\n");
    printf("  -> 3 - Exibir tabela de indice invertido\n");
    printf("  -> 4 - Exibir tabela de repeticao\n");
    printf("  -> 5 - Calcular o tf-idf\n");
    printf("  -> 0 - Sair\n");
    printf("\nOpcao -> ");

    scanf("%d",&op);

    return op;

}

int main()
{
    setlocale(LC_ALL,"");

    int op, quantStop,qtd,k;
    char termo[100];

    criaLista();
    criarStopWords(quantidadeStopWords());
    inserirStopWord();

    strcpy(diretorio,"C:/DiretoriosORI");
    ListaDiretorio(diretorio,"*.txt",OnFind);

    ListaTermos *listaTermos;
    indice *ind;

    docs geral[quantDocumentos()];
    docs *aux;

    qtd = quantidadeTermos();
    listaTermos = (ListaTermos *)malloc(qtd * sizeof(ListaTermos));
    k = insereTermo(listaTermos,qtd);

    ind = (indice *)malloc(k * sizeof(indice));
    calculaIndice(listaTermos,ind,k);

    tf_idf *docsCalculo;
    int tamDocs = docsCalculo = quantDocumentos() * quantidadeTermos();
    docsCalculo = (tf_idf *) malloc(tamDocs * sizeof(tf_idf));

    int contInd = 0;
    int quantTF = 0;



    do
    {
        op = menu();

        switch(op)
        {

        case 1:
            listar();
            break;

        case 2:

            printf("\nTermo -> ");
            scanf("%s",&termo);

            buscarTermo(termo);
            break;

        case 3:

            printf("\n");

            for(int i=0; i<k; i++)
            {
                printf("%s -> ",ind[i].termo);

                for(int j=0; j<ind[i].qtdT; j++)
                {
                    printf("%d ", ind[i].docs[j]);
                }
                printf("\n");
            }
            printf("\n\n");
            system("pause");

            break;

        case 4:

            aux = ListaDocumentos;
            int k = 0;
            char text;
            int n = 0;

            while(aux)
            {
                n = 0;
                for(int i=0; aux->documento[i]!='\0'; i++)
                {
                    text = aux->documento[i];

                    if(text!='!'&&text!='"'&&text!='#'&&text!='$'&&text!='%'&&text!='&'&&text!="'"&&
                            text!='('&&text!=')'&&text!='*'&&text!='+'&&text!=','&&text!='-'&&text!='.'&&
                            text!='/'&&text!=':'&&text!=';'&&text!='<'&&text!='='&&text!='>'&&text!='?'&&
                            text!='@'&&text!='['&&text!=']'&&text!='^'&&text!='_'&&text!='`'&&
                            text!='{'&&text!='|'&&text!='}'&&text!='~'&&text!='\\')
                    {

                        geral[k].documento[n++] = text;
                    }

                }

                k++;
                aux = aux->prox;
            }

            int cont = 0, quant = 0;
            int l = 0;
            int auxiliar = 0;
            char palavra[50];
            int contPalavra = 0;
            int contToken = 0;
            int quantidadeTF = 0;

            while(cont < quantDocumentos())
            {
                geral[l].id = cont;
                quant = 0;
                contToken = 0;
                contPalavra = 0;

                for(int i=0; geral[l].documento[i]!='\0'; i++)
                {
                    if(geral[l].documento[i] == ' ')
                    {
                        quant++;
                    }
                }

                geral[l].token = (tokensDoc *)malloc((quant+1) * sizeof(tokensDoc));
                geral[l].qtdTokens = quant+1;


                for(int i=0; geral[l].documento[i]!='\0'; i++)
                {
                    if(geral[l].documento[i]!=' ')
                    {
                        palavra[contPalavra++] = geral[l].documento[i];
                    }
                    else
                    {
                        strcpy(geral[l].token[contToken++].token,palavra);

                        for(int y=0; palavra[y]!='\0'; y++)
                        {
                            palavra[y] = '\0';
                        }

                        contPalavra = 0;
                    }
                }

                strcpy(geral[l].token[contToken].token,palavra);

                for(int y=0; palavra[y]!='\0'; y++)
                {
                    palavra[y] = '\0';
                }

                l++;
                cont++;

            }

            for(int i=0; i<quantidadeTermos(); i++)
            {
                strcpy(docsCalculo[i].termo,listaTermos[i].termo);
            }

            int i =0;
            int contadorGeral = 0;
            int contadorGlobal = 0;
            int p=0;
            int quantRep = 0;

            while(i < tamDocs)
            {
                docsCalculo[i].docs = (tf_idf *)malloc(quantDocumentos() * sizeof(tf_idf));

                while(p < quantDocumentos())
                {
                    quantRep = 0;

                    for(int j=0; j<geral[contadorGeral].qtdTokens; j++)
                    {
                        if(strcmp(docsCalculo[i].termo,geral[contadorGeral].token[j].token)==0)
                        {
                            quantRep++;

                        }
                    }

                    docsCalculo[i].docs[p].idDoc = geral[contadorGeral].id;
                    docsCalculo[i].docs[p].quant = quantRep;

                    contadorGeral++;
                    p++;
                }

                i++;
                p=0;
                contadorGeral = 0;
            }

            for(int i=0; i<quantidadeTermos(); i++)
            {
                printf("Termo: %s:\n",docsCalculo[i].termo);
                for(int j=0; j<quantDocumentos(); j++)
                {
                    printf("Documento: %d   Quantidade: %d\n",docsCalculo[i].docs[j].idDoc,docsCalculo[i].docs[j].quant);
                }

                printf("\n\n");

            }

            //ListaDocumentos -> Lista com todos os documentos
            //docsCalculo -> Lista de tf-idf

            int co = 0;
            int val = 0;
            int b = 0;
            int cg = 0;

            docs *auxi = ListaDocumentos;

            while(co < quantidadeTermos())
            {
                auxi = ListaDocumentos;

                while(auxi!=NULL)
                {


                    for(int j=0; j<auxi->qtdTokens; j++)
                    {
                        if(strcmp(docsCalculo[co].termo,auxi->token[j].token)==0)
                        {
                            val++;
                            j = auxi->qtdTokens + 1;

                        }
                    }


                    cg++;
                    auxi = auxi->prox;


                }

                //Calculo do tf-idf
                docsCalculo[co].docs[cg].tfIdf = (1 + log10(docsCalculo[co].docs[cg].quant)) * (log10(quantDocumentos()/(float)val));
                val = 0;
                co++;
                cg = 0;

            }

            printf("\n\nIDF\n\n");

             for(int i=0; i<quantidadeTermos(); i++)
            {
                printf("Termo: %s:\n",docsCalculo[i].termo);
                for(int j=0; j<quantDocumentos(); j++)
                {
                    printf("Documento: %d   TF-IDF: %d\n",docsCalculo[i].docs[j].idDoc,docsCalculo[i].docs[j].tfIdf);
                }

                printf("\n\n");

            }

            system("pause");
            break;

        case 5:

            system("pause");
            break;

        case 0:
            break;
        }

    }
    while(op!=0);

}
