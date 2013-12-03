#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>

#include <boost/dynamic_bitset.hpp>

#include "RatingTieBreakingPolicies.h"
#include "../lib/datastructure/Hypergraph.h"

namespace partition {

 typedef hgr::HypergraphType HypergraphType;
 typedef hgr::HypergraphType::HypernodeID HypernodeID;
 typedef hgr::HypergraphType::ContractionMemento Memento;
 typedef hgr::HypergraphType::ConstIncidenceIterator ConstIncidenceIterator;
 typedef hgr::HypergraphType::ConstHypernodeIterator ConstHypernodeIterator;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++" // See Modern C++ Design
template <typename RatingType_, template <class> class _TieBreakingPolicy>
class Coarsener : public _TieBreakingPolicy<HypernodeID>  {
 public:
  typedef RatingType_ RatingType;
  typedef _TieBreakingPolicy<HypernodeID> TieBreakingPolicy;
  
 private:    
  struct HeavyEdgeRating {
    HypernodeID target;
    RatingType value;
    bool valid;
    HeavyEdgeRating(HypernodeID trgt, RatingType val, bool is_valid) :
        target(trgt),
        value(val),
        valid(is_valid) {}
    HeavyEdgeRating() :
        target(std::numeric_limits<HypernodeID>::max()),
        value(std::numeric_limits<RatingType>::min()),
        valid(false) {}
  };
    
 public:  
  Coarsener(HypergraphType& hypergraph, int coarsening_limit, int threshold_node_weight) :
      _hypergraph(hypergraph),
      _coarsening_limit(coarsening_limit),
      _threshold_node_weight(threshold_node_weight),
      _coarsening_history(),
      _tmp_edge_ratings(_hypergraph.initialNumHypernodes(), static_cast<RatingType>(0)),
      _visited_hypernodes(_hypergraph.initialNumHypernodes()),
      _used_entries(),
      _equally_rated_nodes() {}
  
  void coarsen() {
    //    _coarsening_history.push(_hypergraph.contract(0,2));
    forall_hypernodes(hn, _hypergraph) {
      HeavyEdgeRating rating = rate(*hn);
      PRINT("HN " << *hn << ": (" << *hn << "," << rating.target << ")=" << rating.value);
    } endfor
  }

  void uncoarsen() {
    while(!_coarsening_history.empty()) {
      _hypergraph.uncontract(_coarsening_history.top());
      _coarsening_history.pop();
    }
  }

  HeavyEdgeRating rate(HypernodeID u) {
    ASSERT(_used_entries.empty(), "Stack is not empty");
    ASSERT(_visited_hypernodes.none(), "Bitset not empty");
    forall_incident_hyperedges(he,  u, _hypergraph) {
      forall_pins(v, *he, _hypergraph) {
        if (*v != u && belowThresholdNodeWeight(*v, u) ) {
          _tmp_edge_ratings[*v] += static_cast<RatingType>(_hypergraph.hyperedgeWeight(*he))
                                   / (_hypergraph.hyperedgeSize(*he) - 1);
          if (!_visited_hypernodes[*v]) {
            _visited_hypernodes[*v] = 1;
            _used_entries.push(*v);
          } 
        }
      } endfor
    } endfor

    _equally_rated_nodes.clear();
    RatingType tmp = 0.0;
    RatingType max_rating = std::numeric_limits<RatingType>::min();
    while (!_used_entries.empty()) {
      tmp = _tmp_edge_ratings[_used_entries.top()] /
            (_hypergraph.hypernodeWeight(u) * _hypergraph.hypernodeWeight(_used_entries.top()));
      // PRINT("r(" << u << "," << _used_entries.top() << ")=" << tmp); 
      if (max_rating < tmp) {
        max_rating = tmp;
        _equally_rated_nodes.clear();
        _equally_rated_nodes.push_back(_used_entries.top());
      } else if (max_rating == tmp) {
        _equally_rated_nodes.push_back(_used_entries.top());
      }
      _tmp_edge_ratings[_used_entries.top()] = 0.0;
      _visited_hypernodes[_used_entries.top()] = 0;
      _used_entries.pop();
    }
    HeavyEdgeRating ret;
    if (!_equally_rated_nodes.empty()) {
      ret.value = max_rating;
      ret.target = TieBreakingPolicy::select(_equally_rated_nodes);
      ret.valid = true;
    }
    return ret;
  }
  
private:
  bool belowThresholdNodeWeight(HypernodeID u, HypernodeID v) {
    return _hypergraph.hypernodeWeight(v) + _hypergraph.hypernodeWeight(u)
        <= _threshold_node_weight;
  }
  
  HypergraphType& _hypergraph;
  const int _coarsening_limit;
  const int _threshold_node_weight;
  std::stack<Memento> _coarsening_history;
  std::vector<RatingType> _tmp_edge_ratings;
  boost::dynamic_bitset<uint64_t> _visited_hypernodes;
  std::stack<HypernodeID> _used_entries;
  std::vector<HypernodeID> _equally_rated_nodes;
};
#pragma GCC diagnostic pop

} // namespace partition

#endif  // PARTITION_COARSENER_H_
