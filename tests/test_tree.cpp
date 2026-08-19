import std;
import tree;

struct IntHasher {
    std::size_t operator()(const int& value) const {
        return static_cast<std::size_t>(value);
    }
};

int test_tree() {
    modforge::Tree<int> tree;
    auto root = tree.set_root(1);
    auto left = tree.add_child(root, 2);
    auto right = tree.add_child(root, 3);
    auto grand = tree.add_child(left, 4);

    std::vector<int> preorder_values;
    tree.preorder([&](const auto& node) {
        preorder_values.push_back(node->value);
    }, root);
    if (preorder_values.size() != 4) return 1;
    if (preorder_values[0] != 1 || preorder_values[1] != 2 || preorder_values[2] != 4 || preorder_values[3] != 3) return 2;

    std::vector<int> postorder_values;
    tree.postorder([&](const auto& node) {
        postorder_values.push_back(node->value);
    }, root);
    if (postorder_values.size() != 4) return 3;
    if (postorder_values[0] != 4 || postorder_values[1] != 2 || postorder_values[2] != 3 || postorder_values[3] != 1) return 4;

    auto found = tree.find(4);
    if (!found || found->value != 4) return 5;
    if (tree.find(99) != nullptr) return 6;

    modforge::Tree<int> tree2;
    auto root2 = tree2.set_root(10);
    auto child_a = tree2.add_child(root2, 20);
    auto child_b = tree2.add_child(root2, 30);
    tree2.detach_subtree(child_a);
    if (root2->children.size() != 1) return 7;
    if (root2->children[0] != child_b) return 8;
    if (child_a->parent != nullptr) return 9;

    modforge::IndexedTree<int, IntHasher> indexed_tree;
    indexed_tree.set_root(10);
    auto indexed_left = indexed_tree.add_child(indexed_tree[10], 20);
    auto indexed_right = indexed_tree.add_child(indexed_tree[10], 30);
    if (indexed_tree[20] != indexed_left) return 10;
    if (indexed_tree[30] != indexed_right) return 11;

    indexed_tree.detach_subtree(indexed_left);
    if (indexed_tree[20] != nullptr) return 12;
    if (indexed_tree[30] != indexed_right) return 13;

    std::vector<int> indexed_values;
    indexed_tree.preorder([&](const auto& node) {
        indexed_values.push_back(node->value);
    });
    if (indexed_values.size() != 2) return 14;
    if (indexed_values[0] != 10 || indexed_values[1] != 30) return 15;

    return 0;
}
