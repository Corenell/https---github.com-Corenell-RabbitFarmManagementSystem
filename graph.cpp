#include<stdio.h>
#include<stdlib.h>
#include<iostream>

using namespace std;

typedef char gtype;
typedef int EdgeWeight;

#define MAXVEX 100

typedef struct {
    gtype a[MAXVEX];
    EdgeWeight edge[MAXVEX][MAXVEX];
    int vex_num, edge_num;
} graph;

// ����ͼ�ĺ���
void create_graph(graph& M) {
    int vi, vj;
    cout << "Input vex and edge's number please:" << endl;
    cin >> M.vex_num >> M.edge_num;

    // ��ʼ���ڽӾ���
    for (int i = 0; i < M.vex_num; i++) {
        for (int j = 0; j < M.vex_num; j++) {
            M.edge[i][j] = 0;
        }
    }

    // ���붥������
    for (int i = 0; i < M.vex_num; i++) {
        cout << "Input the name of vex " << i + 1 << endl;
        cin >> M.a[i];
    }

    // ����ߵ������յ㣬��ӺϷ�����֤
    for (int i = 0; i < M.edge_num; i++) {
        cout << "Input the start and end of edge " << i << endl;
        cin >> vi >> vj;
        if (vi >= 1 && vi <= M.vex_num && vj >= 1 && vj <= M.vex_num) {  // ��֤����Ķ������Ƿ�Ϸ�
            M.edge[vi - 1][vj - 1] = 1;
        }
        else {
            cout << "Invalid vertex number, please input again." << endl;
            i--;  // �������뵱ǰ�ߵ���Ϣ
            continue;
        }
    }
}

// ���ͼ��Ϣ�ĺ����������ڲ�ѭ������
void pri_graph(graph& M) {
    for (int i = 0; i < M.vex_num; i++) {
        cout << M.a[i] << " ";
    }
    cout << endl;

    for (int i = 0; i < M.vex_num; i++) {
        for (int j = 0; j < M.vex_num; j++) {
            cout << M.edge[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

int main() {
    graph M;
    create_graph(M);
    pri_graph(M);
    return 0;
}