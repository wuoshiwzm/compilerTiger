#ifndef TIGER_COMPILER_LIVENESS_H
#define TIGER_COMPILER_LIVENESS_H

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_{
  G_node src, dst;
  Live_moveList tail;
};
Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);



struct Live_graph { G_graph graph; Live_moveList moves; };

// 指出结点 n 代表的是哪个临时变量，n.info 指向一个 Temp_temp
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);


#endif //TIGER_COMPILER_LIVENESS_H
