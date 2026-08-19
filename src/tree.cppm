/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/19 11:07:44
********************************************************************************/

module;
export module tree;
import std;

NAMESPACE_BEGIN

export template <typename T>
struct TreeNode {
    std::vector<std::shared_ptr<TreeNode>> children{};
    TreeNode* parent{};
    T value{};

    explicit TreeNode(const T &val) : value(val) {  }
    std::shared_ptr<TreeNode> add_child(T value) {
        auto node = std::make_shared<TreeNode>(value);
        node->parent = this;
        children.push_back(node);
        return node;
    }
};

export template <typename T>
class Tree {
    using Node = std::shared_ptr<TreeNode<T>>;
    Node root_{};
public:

    Tree() = default;
    Tree(const Tree &) = default;
    Tree(Tree &&) = default;
    Tree &operator=(const Tree &) = default;
    Tree &operator=(Tree &&) = default;
    ~Tree() = default;

    Node set_root(const T &val) {
        root_ = std::make_shared<TreeNode<T>>(val);
        return root_;
    }

    Node add_child(Node parent, const T &value) {
        return parent->add_child(value);
    }

    void detach_subtree(Node node) {
        if (node->parent == nullptr) {
            root_ = nullptr;
            return;
        }
        auto parent = node->parent;
        auto it = std::ranges::find(parent->children, node);
        parent->children.erase(it);
        node->parent = nullptr;
    }


    void preorder(std::function<void(const Node&)> visit, Node node) {
        if (!node)
            return;

        visit(node);
        for (auto &child : node->children) {
            preorder(visit, child);
        }
    }
    void preorder(std::function<void(const Node&)> visit) {
        preorder(visit, root_);
    }

    void postorder(std::function<void(const Node&)> visit, Node node) {
        if (!node) return;

        for (const auto& child : node->children) {
            postorder(visit, child);
        }
        visit(node);
    }

    void postorder(std::function<void(const Node&)> visit) {
        postorder(visit, root_);
    }

    Node find(const T &value) {
        Node ret;
        preorder([&ret, &value] (Node cur_node) {
            if (cur_node->value == value)
                ret = cur_node;
        });
        return ret;
    }


};


export template <typename T, typename Hasher>
class IndexedTree {
    using Node = std::shared_ptr<TreeNode<T>>;

    Tree<T> tree_{};
    std::unordered_map<std::size_t, Node> hash_index_{};

public:
    IndexedTree() = default;
    IndexedTree(const IndexedTree &) = default;
    IndexedTree(IndexedTree &&) = default;
    IndexedTree &operator=(const IndexedTree &) = default;
    IndexedTree &operator=(IndexedTree &&) = default;
    ~IndexedTree() = default;

    void set_root(const T &value) {
        auto root = tree_.set_root(value);
        Hasher hasher{};
        auto hash = hasher(value);
        hash_index_[hash] = root;
    }

    Node add_child(Node parent, const T& value) {
        auto child = tree_.add_child(parent, value);
        Hasher hasher{};
        hash_index_[hasher(value)] = child;
        return child;
    }

    Node operator[](const T& value) {
        Hasher hasher{};
        auto h = hasher(value);
        auto it = hash_index_.find(h);
        if (it == hash_index_.end())
            return nullptr;
        return it->second;
    }


    void detach_subtree(Node node) {

        Hasher hasher{};
        tree_.postorder([&hasher, this](Node cur_node) {
            auto hash = hasher(cur_node->value);
            hash_index_.erase(hash);
        }, node);
        tree_.detach_subtree(node);
    }

    void preorder(std::function<void(const Node&)> visit) {
        tree_.preorder(visit);
    }

    void postorder(std::function<void(const Node&)> visit) {
        tree_.postorder(visit);
    }

};


NAMESPACE_END
