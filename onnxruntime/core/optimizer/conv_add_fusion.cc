// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/graph/graph_utils.h"
#include "core/optimizer/initializer.h"
#include "core/optimizer/conv_add_fusion.h"

using namespace ONNX_NAMESPACE;
using namespace ::onnxruntime::common;
namespace onnxruntime {

Status ConvAddFusion::Apply(Graph& graph, Node& node, bool& modified, bool& removed) {
  //std::vector<onnxruntime::NodeIndex> removed_nodes;
  //for (auto& node : graph.Nodes()) {

    auto& conv_node = node;
    const auto& add_node = *conv_node.OutputNodesBegin();
    auto* addd_node = graph.GetNode(add_node.Index());

    const auto& conv_inputs = conv_node.InputDefs();
    const auto& add_inputs = add_node.InputDefs();

    const ONNX_NAMESPACE::TensorProto* conv_W_tensor_proto = nullptr;
    graph.GetInitializedTensor(conv_inputs[1]->Name(), conv_W_tensor_proto);

    const ONNX_NAMESPACE::TensorProto* add_B_tensor_proto = nullptr;
    graph.GetInitializedTensor(add_inputs[1]->Name(), add_B_tensor_proto);

    // Currently, fusion is only supported for float or double data type.
    if (!Initializer::IsSupportedDataType(add_B_tensor_proto) ||
        conv_W_tensor_proto->dims_size() < 4 ||
        add_B_tensor_proto->dims_size() != conv_W_tensor_proto->dims_size() - 1 ||
        conv_W_tensor_proto->dims(0) != add_B_tensor_proto->dims(0)) {
      return Status::OK();
    }

    // The dimensions of add_B should be equal to 1 except first dimension.
    for (int i = 1; i < add_B_tensor_proto->dims_size(); i++) {
      if (add_B_tensor_proto->dims(i) != 1) {
        return Status::OK();
      }
    }

    const ONNX_NAMESPACE::TensorProto* conv_B_tensor_proto = nullptr;
    if (conv_inputs.size() == 3) {
      graph.GetInitializedTensor(conv_inputs[2]->Name(), conv_B_tensor_proto);

      if (!Initializer::IsSupportedDataType(conv_B_tensor_proto) ||
          conv_B_tensor_proto->data_type() != add_B_tensor_proto->data_type() ||
          conv_B_tensor_proto->dims_size() != 1 ||
          conv_B_tensor_proto->dims(0) != add_B_tensor_proto->dims(0)) {
        return Status::OK();
      }

      auto conv_B = std::make_unique<Initializer>(conv_B_tensor_proto);
      auto add_B = std::make_unique<Initializer>(add_B_tensor_proto);

      if (conv_B->size() != add_B->size()) {
        return Status::OK();
      }
      // Calculate new value of initializers of conv node
      conv_B->add(*add_B);

      // Create new initializers of conv
      ONNX_NAMESPACE::TensorProto new_conv_B_tensor_proto;
      conv_B->ToProto(&new_conv_B_tensor_proto);

      // Replace initializers of conv node
      graph.RemoveInitializedTensor(conv_inputs[2]->Name());
      graph.AddInitializedTensor(new_conv_B_tensor_proto);
    } else {
      NodeArg* add_B_node_arg = graph.GetNodeArg(add_B_tensor_proto->name());
      if (add_B_node_arg == nullptr) {
        return Status::OK();
      }

      // Update shape of tensor proto
      ONNX_NAMESPACE::TensorProto new_conv_B_tensor_proto(*add_B_tensor_proto);
      int64_t dim = conv_W_tensor_proto->dims(0);
      new_conv_B_tensor_proto.clear_dims();
      new_conv_B_tensor_proto.add_dims(dim);

      graph.RemoveInitializedTensor(add_B_tensor_proto->name());
      graph.AddInitializedTensor(new_conv_B_tensor_proto);

      // Update shape of NodeArg
      TensorShapeProto shape;
      shape.add_dim()->set_dim_value(dim);
      add_B_node_arg->SetShape(shape);

      conv_node.MutableInputDefs().push_back(add_B_node_arg);
      conv_node.MutableInputArgsCount()[2] = 1;
    }

    // Remove Add node.
    if (graph_utils::RemoveSingleInputNode(graph, *addd_node)) {
      modified = true;
    }

  //  removed_nodes.push_back(add_node.Index());
  //}

  //for (auto i : removed_nodes) {
  //  graph.RemoveNode(i);
  //}

  //if (!removed_nodes.empty()) {
  //  modified = true;
  //}

  return Status::OK();
}

bool ConvAddFusion::SatisfyCondition(const Graph& graph, const Node& node) {
  if (!graph_utils::IsSupportedOptypeVersionAndDomain(node, "Conv", 1) ||
      //!graph_utils::IsSupportedProvider(node, GetCompatibleExecutionProviders()) ||
      node.GetOutputEdgesCount() != 1) {
    return false;
  }

  const Node& next_node = *node.OutputNodesBegin();
  if (!graph_utils::IsSupportedOptypeVersionAndDomain(next_node, "Add", 7) ||
      next_node.GetExecutionProviderType() != node.GetExecutionProviderType() ||
      next_node.GetInputEdgesCount() != 1 ||
      graph.IsNodeOutputsInGraphOutputs(next_node)) {
    return false;
  }

  return true;
}

}  // namespace onnxruntime
