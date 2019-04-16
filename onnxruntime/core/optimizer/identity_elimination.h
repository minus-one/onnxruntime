// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/optimizer/rewrite_rule.h"

namespace onnxruntime {

/**
@Class EliminateIdentity

Rewrite rule that eliminates the identity node.

It is attempted to be triggered only on nodes with op type "Identity".
*/
class EliminateIdentity : public RewriteRule {
 public:
  EliminateIdentity() noexcept : RewriteRule("EliminateIdentity", "Eliminate identity node", {"Identity"}) {}

 private:
  bool SatisfyCondition(const Graph& graph, const Node& node) override;

  Status Apply(Graph& graph, Node& node, bool& modified, bool& deleted) override;
};  // namespace onnxruntime

}  // namespace onnxruntime
