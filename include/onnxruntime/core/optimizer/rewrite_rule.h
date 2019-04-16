// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/common/common.h"
#include "core/graph/graph_viewer.h"

namespace onnxruntime {

/**
@class RewriteRule 

The base class for a rewrite rule. A rewrite rule represents a semantics-preserving transformation of a 
computation graph. It can be used to represent, for example, the elimination of operators that serve as 
no-ops (e.g., dropout during inference), as well as inlining of "function" definitions or the dual operation 
of replacing a complex expression by an equivalent function-call). Unlike the more general GraphTransformer, 
a rewrite rule is a more local transformation that is triggered on a particular node of the graph. 

Each rule has a set of conditions and a body. The conditions have to be satisfied for the body of the rule 
to be triggered. Therefore, when creating a new rewrite rule, two main functions have to be implemented: 
- SatisfyCondition defines the condition checks. It is advisable to add the more selective checks first, 
  because those will lead to discarding fast rules that cannot be applied on a node.
- Apply is the actual body of the rule that will be executed if SatisfyCondition returns true for a particular
  node. Note that additional, more complex checks can be included in the Apply if putting them in the
  SatisfyCondition would lead to duplicate work (e.g., when we make a check on a Node attribute but we need
  that attribute to execute the rule too).
In general, simple fast checks are a better fit for SatisfyCondition, whereas more complex ones can be added 
in the Apply.

In order to avoid evaluating the SatisfyCondition for each rule and each node of the graph, in the constructor
of the rewrite rule we can specify the target op types for which a rule will be evaluated. If the op type of a
node is not included in the target op types of a rule, that rule would not be considered at all. If the list of
op types is left empty, that rule will be triggered for every op type.
*/
class RewriteRule {
 public:
  RewriteRule(const std::string& name, const std::string& desc, const std::unordered_set<std::string>& target_op_types)
      : name_(name), desc_(desc), target_op_types_(target_op_types) {
  }

  virtual ~RewriteRule() = default;

  /** Gets the name of this rewrite rule. */
  const std::string& Name() const noexcept {
    return name_;
  }

  /** Gets the description of this rewrite rule. */
  const std::string& Description() const noexcept {
    return desc_;
  }

  const std::unordered_set<std::string>& TargetOpTypes() const noexcept {
    return target_op_types_;
  }

  /** Checks if the condition of the rule is satisfied, and if so applies the body of the rule.
  @param[in] graph The Graph.
  @param[in] node The Node to apply the rewrite to.
  @param[out] modified Set to indicate whether the node was modified or not.
  @param[out] deleted Set to indicate if the node was deleted. 
  @returns Status indicating success or providing error information */
  common::Status CheckConditionAndApply(Graph& graph, Node& node, bool& modified, bool& deleted) {
    return SatisfyCondition(graph, node) ? Apply(graph, node, modified, deleted) : Status::OK();
  }

 private:
  ORT_DISALLOW_COPY_ASSIGNMENT_AND_MOVE(RewriteRule);

  const std::string name_;
  const std::string desc_;

  /** The node op types for which this rule will be triggered. */
  const std::unordered_set<std::string> target_op_types_;

  /** Check if the Node of the given Graph satisfies a condition.
  The rewrite rule is applied if the condition function returns true. This can include
  a more complex pattern matching (conditions on the ascending or descending nodes of the
  node for which this rule was triggered) or some other properties of the nodes. */
  virtual bool SatisfyCondition(const Graph& graph, const Node& node) = 0;

  /**
  Apply the rewrite rule to a specific node.
  The transformation happens in-place. The return-value of node may be different
  from the input-value due to rewriting.
  The value of "modified" indicates if the graph was modified or not. 
  The value of "deleted" indicates if the node was deleted or not. */
  virtual common::Status Apply(Graph& graph, Node& node, bool& modified, bool& deleted) = 0;
};
}  // namespace onnxruntime
