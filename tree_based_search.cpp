#include "build_tree.hpp"
#include <string>
tree_sparql ts;
//节点定义
struct rdf_vertex{
    int vertex_intlable;
    int iteration_count;
    std::vector<match_answer> results;
	std::vector<match_answer> current_results;
	rdf_vertex() {}
    rdf_vertex(int intlable) : vertex_intlable(intlable), iteration_count(0) { }

    void save(graphlab::oarchive& oarc) const{
        oarc << vertex_intlable << iteration_count << results << current_results;
    }

    void load(graphlab::iarchive& iarc){
        iarc >> vertex_intlable >> iteration_count >> results >> current_results;
    }
};

//边定义
class rdf_edge{
public:
    int edge_intlable;
	int graph_id;

	rdf_edge() {}
    rdf_edge(int intlable, int id) : edge_intlable(intlable), graph_id(id){ }

    void save(graphlab::oarchive& oarc) const{
        oarc << edge_intlable << graph_id;
    }

    void load(graphlab::iarchive& iarc){
        iarc >> edge_intlable >> graph_id;
    }
};

typedef graphlab::distributed_graph<rdf_vertex, rdf_edge> graph_type;

class tree_based_search :
            public graphlab::ivertex_program<graph_type, graphlab::empty, scatter_type>{
private:
	scatter_type st;
	bool scatter_current_results;
public:
	void init(icontext_type& context, const vertex_type& vertex, const scatter_type& msg) {
		st = msg;
  	}

	//gather过程无操作
	edge_dir_type gather_edges(icontext_type& context, const vertex_type& vertex) const {
    	return graphlab::NO_EDGES;
	}
  
	void apply(icontext_type& context, vertex_type& vertex, const gather_type& total) {

		//若迭代次数超过树的高度 结束
		if(vertex.data().iteration_count >= ts.height){
			//把匹配根节点的结果从current_results加入到results里
			std::cout << "message from vertex id: " << vertex.id() << " : iteration_count >= ts.height\n";
			return;	
		}
	
		//若是迭代的第一层 则让本点与查询图中的所有点进行匹配 并把有效结果存到节点的 vertex.data().current_results里面
		if(vertex.data().iteration_count == 0){
			scatter_current_results = true;
			unsigned int i;
			for(i = 0; i < ts.vertices.size(); ++ i){
				if((ts.vertices[i].vertex_value == ALL_MATCH)
				|| (ts.vertices[i].vertex_value == vertex.data().vertex_intlable)){
					vertex.data().current_results.push_back(match_answer(ts.vertices[i].vertex_id, vertex.id()));	
				}	
			}
			//把作为根节点的匹配结果加入到最终结果里
			for(i = 0; i < vertex.data().current_results.size(); ++ i){
				if(vertex.data().current_results[i].id == ts.root_id){
					vertex.data().results.push_back(vertex.data().current_results[i]);
					break;
				}					
			}
		}
		else{
			scatter_current_results = false;
			for(unsigned int i = 0; i < st.match_answers.size(); ++ i){
				vertex.data().results.push_back(st.match_answers[i]);
			}
		}

		++ vertex.data().iteration_count;

		//还剩下 如何通过笛卡尔积还原匹配结果 = =

	}
	
	//先返回所有边 具体在scatter函数里再判断
	edge_dir_type scatter_edges(icontext_type& context, const vertex_type& vertex) const {
		return graphlab::ALL_EDGES;
	}

	void scatter(icontext_type& context, const vertex_type& vertex, edge_type& edge) const {
			for(unsigned int i = 0; i < vertex.data().current_results.size(); ++ i){
				//若方向不匹配 则不发送
				match_answer ma = vertex.data().current_results[i]; 
				if(((ts.vertices[ma.id].edge_dir == NONE) || (ts.vertices[ma.id].edge_dir == DIR_IN && edge.target().id() != vertex.id()) 
				|| (ts.vertices[ma.id].edge_dir == DIR_OUT && edge.source().id() != vertex.id()))){
					continue;		
				}

				//若边的标签不匹配 则不发送
				if((ts.vertices[ma.id].edge_value != ALL_MATCH) 
				&& (ts.vertices[ma.id].edge_value != edge.data().edge_intlable)){
					continue;
				}

				//若父节点的标签不匹配 则不发送
				if(ts.vertices[ma.id].father_vertex_value != ALL_MATCH){
					if(((ts.vertices[ma.id].father_vertex_value != edge.target().data().vertex_intlable && edge.source().id() == vertex.id())
					|| (ts.vertices[ma.id].father_vertex_value != edge.source().data().vertex_intlable && edge.target().id() == vertex.id()))){
						continue;
					}
				}

				//若是第一层迭代 满足发送条件 则向目标发送匹配数据
				if(scatter_current_results == true){
					scatter_type to_scatter;
					to_scatter.match_answers.push_back(ma);

					if(edge.source().id() == vertex.id())
						context.signal(edge.target(), to_scatter);
					else
						context.signal(edge.source(), to_scatter);
				}

				//若非第一层迭代 判断ma是否为叶子节点 若是则不发送
				else{
					if(ts.vertices[ma.id].is_leaf == true){
						continue;
					}
					else{
						 if(edge.source().id() == vertex.id())
                        		context.signal(edge.target(), st);
                    		else
                        		context.signal(edge.source(), st);
					}
				}	
			}
		}

		void save(graphlab::oarchive& oarc) const{
        	oarc << st  << scatter_current_results ;
    	}

   		void load(graphlab::iarchive& iarc){
        	iarc >> st >> scatter_current_results ;
    	}
		
}; 

bool line_parser(graph_type& graph, const std::string& filename, const std::string& textline){
    std::stringstream strm(textline);
    graphlab::vertex_id_type subject_unique_id;
    graphlab::vertex_id_type subject_intlable;
    graphlab::vertex_id_type predicate;
    graphlab::vertex_id_type object_unique_id;
    graphlab::vertex_id_type object_intlable;
    graphlab::vertex_id_type graph_id;

    strm >> subject_unique_id >> subject_intlable >> predicate
         >> object_unique_id >> object_intlable >> graph_id;
    //添加两个点
    if(!graph.contains_vertex(subject_unique_id))
        graph.add_vertex(subject_unique_id, rdf_vertex(subject_intlable));
    if(!graph.contains_vertex(object_unique_id))
        graph.add_vertex(object_unique_id, rdf_vertex(object_intlable));
    //添加这条边
    if(subject_unique_id != object_unique_id)
        graph.add_edge(subject_unique_id, object_unique_id, rdf_edge(predicate, graph_id));
  return true;
}


std::vector<std::vector<match_answer> > get_cartesian_product(std::vector<std::vector<match_answer> > & c1, 
																std::vector<std::vector<match_answer> > & c2){
	if(c1.size() == 0)
		return c2;
	std::vector<std::vector<match_answer> > ret;
       for(unsigned int i = 0; i < c1.size(); ++ i){
           for(unsigned int j = 0; j < c2.size(); ++ j){
				std::vector<match_answer> to_record;
               	to_record.insert(to_record.begin(), c1[i].begin(), c1[i].end());
              	to_record.insert(to_record.begin(), c2[j].begin(), c2[j].end());
              	ret.push_back(to_record);
          }
      }
      return ret;
}


class graph_writer{
public :
    std::string save_vertex(graph_type::vertex_type v) {
        std::stringstream output;
        std::vector<match_answer> res = v.data().results;
		std::vector<match_answer> cur_res = v.data().current_results;
		std::vector<std::vector<std::vector<match_answer> > > fi_res;
	/*	output << "$$$$$$$$$$$$$$$\n";
        output << "vertex id: " << v.id() << "\n";
		output << "results:\n";
        for(unsigned int i = 0; i < res.size(); ++ i){
            output << "id: " << res[i].id << " cand: " << res[i].candidate  << " | ";
        }

		output << "\n";
		
		output << "current_results:\n";
		for(unsigned int j = 0; j < cur_res.size(); ++ j){
			output << "id: " << cur_res[j].id << " cand: " << cur_res[j].candidate << " | ";
		}

        output << "\n";*/

		//先把匹配结果按照查询图ID分组 若任何一组size为0就结束
		std::vector<std::vector<match_answer> > mas;
		std::vector<match_answer> ma;
		for(unsigned int k = 0; k < ts.vertices.size(); ++ k){
			mas.clear();
			for(unsigned int m = 0; m < res.size(); ++ m){
				ma.clear();
				if(res[m].id == k){
					ma.push_back(res[m]);
					mas.push_back(ma);
				}
			}
			if(mas.size() == 0){
				//output << "***************\n"; 
				return output.str();
			}
			fi_res.push_back(mas);
		}		

		//若此点的数据包含查询图里所有点的匹配结果 则按照笛卡尔积计划得到最终结果
		for(unsigned int n = 0; n < ts.cartesian_product_plan.size(); ++ n){
			//按照笛卡尔积计划 先让每一行里的所有点按顺序作笛卡尔己 然后把结果保存在最后一个点里
			std::vector<std::vector<match_answer> > product_result;
			for(unsigned int m = 0; m < ts.cartesian_product_plan[n].size(); ++ m){
				product_result = get_cartesian_product(product_result, fi_res[ts.cartesian_product_plan[n][m]]);
			}
			fi_res[ts.cartesian_product_plan[n][ts.cartesian_product_plan[n].size() - 1]] = product_result;
		}

		int xx = ts.cartesian_product_plan.size();
		int x = ts.cartesian_product_plan[xx- 1].size();

		int root_num = ts.cartesian_product_plan[xx - 1][x - 1];
		output << "final_results:\n";
		for(unsigned int p = 0; p < fi_res[root_num].size(); ++ p){
			for(unsigned int q = 0; q < fi_res[root_num][p].size(); ++ q){
				output << "id: " << fi_res[root_num][p][q].id << " cand: " << fi_res[root_num][p][q].candidate << " | ";
			}
			output << "\n";
		}

		//output << "***************\n"; 
        return output.str();
	}

    std::string save_edge(graph_type::edge_type e) {
        return "";
    }
};

int main(int argc, char** argv) {

	graphlab::mpi_tools::init(argc, argv);
    graphlab::distributed_control dc;
    graphlab::timer timer;
	const char * filename = argv[1];
	std::string input_graph_dir = argv[2];
	std::string output_result_dir = argv[3];

	sparql_to_tree stt;
    timer.start();
	ts = stt.get_tree_sparql(filename);

	dc.cout() << "tree sparql:\n";
	for(unsigned int i = 0; i < ts.vertices.size(); ++ i){
		single_vertex sv = ts.vertices[i];
		dc.cout() << "id: " << sv.vertex_id << " val: " << sv.vertex_value << " fa: " << sv.father_vertex_value;
		dc.cout() << " ev: " << sv.edge_value << " ed: " << sv.edge_dir << " leaf:" << sv.is_leaf << "\n"; 
	}
	dc.cout() << "plan:\n";
	for(unsigned int i = 0; i < ts.cartesian_product_plan.size(); ++ i){
		for(unsigned int j = 0; j < ts.cartesian_product_plan[i].size() ;++ j){
			dc.cout() << ts.cartesian_product_plan[i][j] << " ";
		}
		dc.cout() << "\n";
	}
	dc.cout() << "height: " << ts.height << "\n";
	dc.cout() << "root id: " << ts.root_id << "\n";

	

	dc.cout() << "sparql tree built: " << timer.current_time_millis() << "ms\n";

    graph_type graph(dc);
    graph.load(input_graph_dir, line_parser);

	dc.cout() << "graph loaded: " << timer.current_time_millis() << "ms\n";

    graph.finalize();

	dc.cout() << "graph finalized: " << timer.current_time_millis() << "ms\n";

    graphlab::omni_engine<tree_based_search> engine(dc, graph, "sync");
    engine.signal_all();
    engine.start();

	dc.cout() << "engine finished: " << timer.current_time_millis() << "ms\n";

    graph.save(output_result_dir, graph_writer(), false, true, false);

	dc.cout() << "results saved: " << timer.current_time_millis() << "ms\n";

    graphlab::mpi_tools::finalize();
	
	dc.cout() << "finish time: " << timer.current_time_millis() << "ms\n";

    return 0;
	
}
