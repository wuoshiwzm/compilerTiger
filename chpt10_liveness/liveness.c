#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcpy */
#include "util.h"
#include "table.h"
#include "table.c"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "errormsg.h"
#include "assem.h"
#include "graph.h"
#include "temp.h"
#include "flowgraph.h"
#include "liveness.h"

#define TABSIZE 127

// 将节点加入控制流图
static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps);

// 查看一个节点有哪些活跃的临时变量
static Temp_tempList lookupLiveMap(G_table t, G_node flownode);

Temp_tempList tempListMinus(Temp_tempList a, Temp_tempList b);

Temp_tempList tempListUnion(Temp_tempList a, Temp_tempList b);

// 判断两个 出口/入口活跃表 是否相等
// bool G_table_equal(G_table t1, G_table t2);


/**
 * 计算出完整的 liveMap 后，便可以构建冲突图。对于流图中的每个结点 node, 若该结点有新定值的临时变量 d in def(node),
 * 并且有属于 liveMap 的临时变量 {t1, t2, ...} ,则添加冲突边 (d, t1), (d, t2), ...
 * 对于 MOVE 指令，添加这些边是安全的但不是
 */
// 通过流图 G_graph flow 生成  live map， 记录每个节点出口活跃的的变量
// construct live map: remembers what is live at the exit of each flow-graph node

// 构造活跃表 （计算所有节点的入口活跃  出口活跃 表）
// struct Live_graph { G_graph graph; Live_moveList moves; };
struct Live_graph Live_liveness(G_graph flow){

  /** 迭代更新结点的 出口活跃/入口活跃 变量集 ins[],outs[]
      for each n
        in[n] ← { }; out[n] ← { }
      repeat
        for each n
          in′[n] ← in[n]; out′[n] ← out[n]
          in[n] ← use[n] ∪ (out[n] − def[n])
          out[n] ← ∪ (s∈succ[n]) in[s]

      // 不断更新，直到 出口活跃，入口活跃 不再变化
      until in′[n] = in[n] and out′[n] = out[n] *** for all n ***
   */

  /* 0. in[n] ← { }; out[n] ← { }
   *    TAB_table_  *G_table; 入口活跃， 出口活跃
   */
  G_table in = G_empty(), out = G_empty();

  // 循环更新 in, out , 直到 不再变化
  {
    G_table in1 = G_empty(), out1 = G_empty(); // 初始化 in' out'
    bool stop = TRUE;

    // 遍历所有节点
    do {

      // 进行一次遍历
      for (G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
        G_node n = nodes->head;

        /* 1. in′[n] ← in[n]; out′[n] ← out[n]
         *    enterLiveMap(G_table t, G_node flownode, Temp_tempList temps)
         */
        enterLiveMap(in1, n, lookupLiveMap(in, n));
        enterLiveMap(out1, n, lookupLiveMap(out, n));

        {
          Temp_tempList n_out = lookupLiveMap(out, n);

          /* 2. in[n] ← use[n] ∪ (out[n] − def[n])
           *    更新入口活跃
           */
          Temp_tempList n_in = tempListUnion(FG_use(n), tempListMinus(n_out, FG_def(n)));

          /* 3. out[n] ← ∪ (s∈succ[n]) in[s]
           *    更新出口活跃, n 节点
           */
          for( G_nodeList ss = FG_succ(n); ss;ss = ss->tail ){
            G_node s = ss->head;
            enterLiveMap(out, n, lookupLiveMap(in, s));
          }
        }

        // // 使用
        // Temp_tempList ins = FG_use(n);
        // Temp_tempList last = NULL;
        // for(Temp_tempList l = ins; l; l = l->tail) last = l;
        // // 定值
        // Temp_tempList a,b,c;
        // TAB_table t = TAB_empty();
        // for(Temp_tempList l = FG_def(n); l; l= l->tail) TAB_enter(t, l->head, (void *) TRUE);
      }

      /* 4. until in′[n] = in[n] and out′[n] = out[n] *** for all n ***
       *    判断入口/出口活跃度表不再变化
       */
      int loop = 1;
      for (G_nodeList nodes = G_nodes(flow); nodes && loop == 1; nodes = nodes->tail) {
        G_node n = nodes->head;
        // 获取 in[n]，in′[n] static Temp_tempList lookupLiveMap(G_table t, G_node flownode
        Temp_tempList in_n = lookupLiveMap(in, n);
        Temp_tempList in1_n = lookupLiveMap(in1, n);

        // 获取 out′[n]
        Temp_tempList out_n = lookupLiveMap(out, n);
        Temp_tempList out1_n = lookupLiveMap(out1, n);

        // 只要有一个节点的 in, out 变化为， 就要继续遍历
        if (chk_G_table_change(in1, in) || chk_G_table_change(out1, out)){
           stop = FALSE;
           loop = 0;
        }
      }
    } while (!stop);
    free(in1), free(out1);
  }


  // 生成出口活跃表 out_table
}


bool chk_G_table_change(Temp_tempList t1, Temp_tempList t2){
  
}




// Temp_tempList a - Temp_tempList b
Temp_tempList tempListMinus(Temp_tempList a, Temp_tempList b){

  return NULL;
}

// Temp_tempList a + Temp_tempList b
Temp_tempList tempListUnion(Temp_tempList a, Temp_tempList b){

  return NULL;
}



// Live_moveList 构造函数
Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail){
  Live_moveList ml = (Live_moveList)checked_malloc(sizeof(ml));
  ml->src = src;
  ml->dst = dst;
  ml->tail = tail;
  return ml;
}

// 指出结点 n 代表的是哪个临时变量，n.info 指向一个 Temp_temp
Temp_temp Live_gtemp(G_node n){
  return (Temp_temp)G_nodeInfo(n);
}

// 把 node->info(temps) 添加入表 t 中
static void enterLiveMap(G_table t, G_node flownode, Temp_tempList temps){
  G_enter(t, flownode, temps);
}

static Temp_tempList lookupLiveMap(G_table t, G_node flownode){
  return (Temp_tempList)G_look(t, flownode);
}



