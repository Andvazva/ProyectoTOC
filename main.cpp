
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <tgmath.h>
#include <vector>
#include <algorithm>

#define NUM_SECONDS_TO_WAIT 10 // Time limit para ATB

using namespace std;
using namespace std::chrono;


struct node{
    int id;                 // Id del nodo
};

struct modelo{
    int id;             // Id del nodo
    float price = 0;    // Precio del router
    float pfail = 0;    // Probabilidad de fallo del router
};


struct estado{
    vector<int> solution;   // Vector que contiene los modelos asignados a los routers en un estado
    double f;
};


float probTotal = 0;    // Probabilidad de fallo total de los modelos de routers


// BFS modificado para encontrar el número de islas teniendo en cuenta nodos desconectados
void BFS(vector< list< int > > adj,vector< int > &res, estado est, int s, int modelDesc) {
    int V = adj.size();
    bool *visited = new bool[V];

    for(int i = 0; i < V; i++)
        visited[i] = false;

    // Cola del bfs
    list<int> queue;

    // Añadir el primer nodo a la lista de visitados
    visited[s] = true;
    queue.push_back(s);

    list<int>::iterator i;  // Iterador para recorrer la lista de adyacencia

    while(!queue.empty()) {
        s = queue.front();
        res[s] = 1;
        queue.pop_front();  // Se saca el primer elemento
        // Se recorren todos los vértices adyacentes que no hayan sido visitados
        for (i = adj[s].begin(); i != adj[s].end(); ++i) {
            if (!visited[*i]) { // Si no está en la lista de visitados y si su modelo es diferente al modelo eliminado
                visited[*i] = true;
                if (est.solution[*i]!=modelDesc)
                    queue.push_back(*i);        // Se añade a la cola
            }
        }
    }
}


// Obtener el número de componentes conexas
int compConex(vector< list< int > > adj, estado e, int modelDesc){
    int ini = 0; // Primer valor desde el que se realiza el bfs
    vector<int> res(adj.size(),0);  // Resultado del bfs
    int ccn = 1; // Número de componentes conexas

    // Este bucle busca un nodo de inicio que no esté desconectado
    if(e.solution[ini]!=-1) { // Si es -1, es un cliente y se puede comenzar el recorrido desde un cliente siempre
        while (ini < adj.size() && e.solution[ini] == modelDesc) {
            ini++;
        }
    }

    // Se realiza el primer BFS
    BFS (adj,res,e,ini,modelDesc);

    for (int i = 0; i<res.size(); i++) {
        if (res[i] == 0 && e.solution[i]!= modelDesc) {
            BFS(adj, res, e, i, modelDesc);
            ccn++;
        }
    }
    return ccn;
}


// Inicialización aleatoria
estado iniRand(int numModelos,vector< node > &nodes){
    int tam = nodes.size();
    vector<int> sol(tam);   // Vector de la solución
    estado est;
    est.solution = sol;

    for (int i = 0; i<tam; i++){
        int r = rand()%(((numModelos-1) - 0) + 1) + 0;  // Número aleatorio entre 0 y el último valor de modelos
        est.solution[i] = r;  // Router
    }
    return est;
}


// Función de evaluación
float fitness(vector< list< int > > &adj,estado &est, vector<modelo> &modelos, int budget){
    int cost = 0;           // Budget
    int i = 0;
    float pfail;            // probabilidad de fallo
    float npfail;           // probabilidad de no fallo
    int ccn;                // componentes conexas
    float resultado = 0;

    //Se va calculando el precio de todos los routers hasta que sobrepase el presupuesto
    while ( (i < est.solution.size()) && (cost <= budget) ){
        cost = cost + modelos[est.solution[i]].price;
        i++;
    }
    if (cost <= budget){
        //Número de islas sin fallo
        //Se calcula el número de componentes conexas desconectando cada grupo de modelos
        for (int i = 0; i<modelos.size(); i++){
            ccn = compConex(adj,est,i);
            pfail = modelos[i].pfail;
            //npfail = 1-pfail;
            resultado += (pfail/probTotal)*ccn;    //(ccn*pfail);//*(ccnNoFail*npfail);
        }
    }
    est.f = resultado;
    return resultado;
}


// Función para visualizar la lista de adyacencia
void printAdjList(vector< list< int > > adjList) {
    for (int i = 0; i < adjList.size(); ++i) {
        cout << "Nodo: "<< i;
        list< int >::iterator itr = adjList[i].begin();

        while (itr != adjList[i].end()) {
            cout <<" -> "<< (*itr);
            ++itr;
        }
        cout << endl;
    }
}


// Función que crea un vector de nodos
vector<node> createNodesVector(int numNodes){
    vector<node> nodes;
    for (int i = 0; i<numNodes; i++){
        node n;
        n.id=i;
        nodes.push_back(n);
    }
    return nodes;
}


/* Simulated Annealing
*  Entradas:
*  adj: Lista de adyacencia
*  modelos: Vector de modelos
*  sO: El estado inicial inicializado aleatoriamente
*  kmax: El máximo número de pasos
*  budget: Presupuesto
*  nodos: vector de nodos
*  t1: ATB
*
*  Salida: Vector de estados*/
estado SA(vector< list< int > > &adj, vector<modelo> &modelos, estado &s0, float kmax, int budget,vector<node> &nodos, time_t t1){
    estado s = s0;
    estado snew;
    estado sMin = s0;
    float Es;       // Función de evaluación de s
    float Esnew;    // Función de evaluación de snew
    float EsMin = INTMAX_MAX; // Evaluación mínima
    float T = 1;      // Temperatura
    float tmin = 0.001;
    float alfa = 0.9;
    float P = 0;    // Probabilidad de intercambiar el estado

    cout << "Iterando, por favor espere..."<< endl;
    while (T>tmin){
        for (int i = 0; i<kmax; i++) {

            Es = fitness(adj, s, modelos, budget);  // Función de evaluación del estado
            // Generar una configuración inicial (vecino) correcta
            while (Es == 0 ) {                       // Si la función de evaluación es 0 --> sobrepasa el coste
                s = iniRand(modelos.size(), nodos);
                Es = fitness(adj, s, modelos, budget);
            }

            cout << Es << endl;
            if (Es < EsMin) {   // Si la f.eval es menor que la eval mín, se asigna como mín, y se asigna el estado
                EsMin = Es;
                sMin = s;
            }

            if (EsMin <= 1) break;

            // Generar un vecino que no tenga coste = 0
            snew = iniRand(modelos.size(),nodos);

            Esnew = fitness(adj, snew, modelos, budget);    // Función de evaluación del estado nuevo
            while (Esnew == 0) {                            // Si la función de evaluación es 0 --> sobrepasa el coste
                snew = iniRand(modelos.size(), nodos);
                Esnew = fitness(adj, snew, modelos, budget);
            }

            if (Esnew<Es)   // Comparamos las funciones de evaluación
                s = snew;   // Asignamos el nuevo estado como inicial
            else {
                P = exp((Es - Esnew) / T);  // Función exp de SA
                //cout<<"Probabilidad "<<T<<endl;
                float r = ((double) rand() / (double) (RAND_MAX));  //Se genera un aleatorio entre 0 1

                if (P > r)      // Si la probabilidad sobrepasa el # aleatorio
                    s = snew;   // Asignamos en nuevo estado como inicial
            }

            time_t t2 = time(0);    // Cálculo de tiempo para ATB
            if ((t2 - t1) >= NUM_SECONDS_TO_WAIT) {
                break;
            }
        }
        if (EsMin <= 1) break;
        T *= alfa;  // Decrementar temperatura

    }
    cout << endl << "Evaluación mínima: " << EsMin;
    return sMin;
}


int main(int argc, char *argv[]){

    srand(time(NULL));      // Inicialización semilla aleatoria

    ifstream infile;

    infile.open(argv[1]);   // Fichero de entrada

    int totalNodes;         // Número total de nodos
    int numE;               // Número de aristas
    int budget;             // Presupuesto total
    int numModels;          // Tipos de modelos de routers

    string line;            // Cada línea del fichero

    // Cálculo de tiempos para ATB
    high_resolution_clock::time_point tt1 = high_resolution_clock::now();   // HRC
    clock_t tStart = clock();
    time_t t1 = time(0);

    /******************************** LEER FICHERO ENTRADA ***************************************/
    // Se leen el número total de nodos
    getline(infile, line);
    istringstream iss(line);
    iss >> totalNodes;
    cout << "Número de nodos: " << totalNodes << endl;

    // Se crea el vector de nodos
    vector<node> nodes = createNodesVector(totalNodes);

    // Se crea la lista de adyacencia que es un vector de listas, cada elemento es una lista de <int>
    vector<list<int> > adjacencyList(totalNodes);

    // Se leen el número de aristas
    getline(infile, line);
    istringstream iss4(line);
    iss4 >> numE;
    cout << "Número de aristas: " << numE << endl;

    // Se lee el grafo
    for (int i = 0; i < numE; i++) {
        getline(infile, line);
        istringstream iss(line);
        int v1, v2;
        iss >> v1;
        iss >> v2;
        adjacencyList[v1].push_back(v2);
        adjacencyList[v2].push_back(v1);
    }

    // Se lee el presupuesto
    getline(infile, line);
    istringstream iss5(line);
    iss5 >> budget;
    cout << "Presupuesto total: " << budget << endl;

    // Se lee el número total de modelos (tipos de router)
    getline(infile, line);
    istringstream iss6(line);
    iss6 >> numModels;
    cout << "Tipos de router: " << numModels << endl << endl;

    // Vector de modelos del router
    vector<modelo> modelos(numModels);

    // Leer precio y probabilidad de fallo
    for (int i = 0; i < numModels; i++) {
        getline(infile, line);
        istringstream iss(line);
        iss >> modelos[i].id;
        iss >> modelos[i].price;
        iss >> modelos[i].pfail;
    }

    infile.close(); // Cerrar fichero

    // Cálculo de la probabilidad de fallo total de los modelos de routers
    for(int i = 0; i<modelos.size(); i++){
        probTotal += modelos[i].pfail;
    }

    // Mostrar el grafo completo (lista de adyacencia)
    cout << "Grafo" << endl;
    printAdjList(adjacencyList);
    cout << endl;

    /***************************** BEGIN ***********************************/
    // Inicialización
    estado ini = iniRand(numModels, nodes);

    // Imprimir el resultado por pantalla
    cout << endl << "Asignaciones después de la inicialización aleatoria" << endl;
    for (int i = 0; i < ini.solution.size(); i++) {
        cout << "Router: " << i << " -> ";
        cout << "Modelo: " << ini.solution[i] << endl;

    }
    cout << endl;

    // Simulated Annealing
    estado e = SA(adjacencyList, modelos, ini, 150, budget, nodes, t1);

    // Visualizar solución final
    cout << endl << endl;
    for (int i = 0; i < e.solution.size(); i++) {
        cout << "Nodo: " << i << " -->" << e.solution[i] << endl;
    }


    // Cálculo de tiempos
    clock_t tEnd = clock();
    printf("Time taken: %.2fs\n", (double)(tEnd - tStart)/CLOCKS_PER_SEC);

    high_resolution_clock::time_point tt2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( tt2 - tt1 ).count();
    cout << "High Res Clk: " << duration;

    return 0;
}
