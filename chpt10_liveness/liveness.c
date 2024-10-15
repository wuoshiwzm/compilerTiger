#include "liveness.h"
#include "graph.h"



// 一个结点有哪些出口活跃的临时变量
// 将节点加入控制流图
static void enterLiveMap(G_table t, G_node flownode, Temp_tempList temps){
  G_enter(t, flownode, temps);
}

static Temp_tempList lookupLiveMap(G_table t, G_node flownode){
  return (Temp_tempList)G_look(t, flownode);
}
