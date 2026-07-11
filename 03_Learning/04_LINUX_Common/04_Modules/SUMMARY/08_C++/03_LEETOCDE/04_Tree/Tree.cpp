#include <iostream>
#include <vector>
#include <stack>
#include <climits>
using namespace std;
 
struct TreeNode
{
    int val;
    TreeNode *left;
    TreeNode *right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};
 
struct TreeNode *CreateTree(std::vector<int> nums)
{
    std::vector<TreeNode *> vecNodes;
    struct TreeNode *root = new TreeNode(nums.at(0));
    vecNodes.push_back(root);
 
    for (int i = 1; i < nums.size(); i++)
    {
        if (nums[i] != -1)
        {
            TreeNode *node = new TreeNode(nums.at(i));
            if (i & 0x1)
            {
                vecNodes[(i - 1) / 2]->left = node;
            }
            else
            {
                vecNodes[(i - 1) / 2]->right = node;
            }
            vecNodes.push_back(node);
        }
    }
 
    return root;
}
 
void DeleteTree(struct TreeNode *root)
{
    if (root == NULL)
        return;
 
    // std::cout << "Free " << root->val << std::endl;
    //PrintRootLR(root, root->left, root->right);
    //PrintRootLRVal(root->val, );
 
    DeleteTree(root->left);
    DeleteTree(root->right);
 
    //std::cout << "Free " << root->val << std::endl;
    //PrintRootLR(root, root->left, root->right);
    delete root;
}
 
std::vector<int> inorderTraversal(TreeNode *root)
{
    std::vector<int> result;
    std::stack<TreeNode *> stk;
    TreeNode *curr = root;
 
    while (curr or !stk.empty())
    {
        while (curr)
        {
            stk.push(curr);
            curr = curr->left;
        } // push all left
 
        // adapt curr to top()
        curr = stk.top();
 
        std::cout << curr->val << std::endl;
 
        // add result
        result.push_back(curr->val);
        stk.pop();
 
        // get right node
        curr = curr->right;
    }
 
    return result;
}
 
void printVector(const std::vector<int> &vec)
{
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        std::cout << vec[i];
        if (i < vec.size() - 1)
            std::cout << ",";
    }
    std::cout << "]" << std::endl;
}
 
void printTree(TreeNode *root, std::string prefix = "", bool isLeft = true)
{
    if (root == nullptr)
        return;
    std::cout << prefix;
    std::cout << (isLeft ? "├── " : "└── ");
    std::cout << root->val << std::endl;
    // Recur for left and right
    printTree(root->left, prefix + (isLeft ? "│   " : "    "), true);
    printTree(root->right, prefix + (isLeft ? "│   " : "    "), false);
}

/*
    0. Check l_val < root_val < r_val

    1. 
          5
      ┌───┴───┐
      1       7
    ┌─┴─┐   ┌─┴─┐
            6   8             -> valid
                                
    2. Apply MAX,MIN with pre_order
        + Check - MIN < 5 < MAX
        + Check - MIN < 1 < 5
        ...
        + Check - 7   < 8 < MAX
        -> all checked -> valid
*/
bool dfs(TreeNode* node, long long low, long long high) {
    if (!node) return true;
 
    //std::cout << node->val << ", " << low << ", " << high << std::endl;
 
    if (node->val <= low || node->val >= high)
        return false;
 
    return dfs(node->left, low, node->val) &&
           dfs(node->right, node->val, high);
}
 
bool isValidBST(TreeNode* root) {
    return dfs(root, LLONG_MIN, LLONG_MAX);
}
 
/*
    0. Check subtress has the same depth or difference by one

    1. 
          5
      ┌───┴───┐
      1       7
    ┌─┴─┐   ┌─┴─┐
            6   8             -> valid - differed by one
                                
    2. Check subtree with its node having the same depth
       Using the postorder incretmented each return 
*/
int DepthOfTree(TreeNode* root){
    if (!root) return 0;
    int max_l, max_r;

    max_l = DepthOfTree(root->left);
    if (max_l == -1) return -1;
    max_r = DepthOfTree(root->right);
    if (max_r == -1) return -1;

    // std::cout << "val = " << root->val << " -> ";
    // std::cout << "[" << max_l << "," << max_r << "]" << std::endl;

    if (abs(max_l - max_r) > 1){
        return -1;
    }

    return std::max(max_l, max_r) + 1;
}

bool CheckBalancedTree(TreeNode* root){
    int res = DepthOfTree(root);
    if (res == -1) return false;
    else return true;
}

/*
    0. List all paths from root to leaf (no childs)

    1. 
          5
      ┌───┴───┐
      1       7
    ┌─┴─┐   ┌─┴─┐
            6   8             5 -> 1
                              5 -> 7 -> 6
                              5 -> 7 -> 8
                                
    2. Each recursion append val and  '->'
*/
void PreOrder_PathLeafth(TreeNode* node, std::string path, std::vector<string>& vec_str){

    path += std::to_string(node->val);
    if (!node->left and !node->right){
        vec_str.push_back(path);
        return;
    }

    path += "->";
    PreOrder_PathLeafth(node->left, path, vec_str);
    PreOrder_PathLeafth(node->right, path, vec_str);
}

void PathLeafth(TreeNode* root, std::vector<string>& vec_str){
    PreOrder_PathLeafth(root, "", vec_str);
}

int main()
{
    // create tree
    std::vector<std::vector<int>> vector_arr = {
        {5,1,7},                        // 0
        {5,1,7,-1,3,-1,8,2},            // 1
        {5,1,7,6,8},                    // 2
        {5,1,7,6,8,-1,-1,-1,-1,2,3},    // 3
        {5,1,7,-1,-1,6,8}               // 4
    };

    // choose tree here
    TreeNode *root = CreateTree(vector_arr[3]);
    printTree(root);
 
    // ====== main ======
    bool res = isValidBST(root);
    std::cout << "1. isValidBST: " << res << std::endl;

    res = CheckBalancedTree(root);
    std::cout << "2. CheckBalancedTree: " << res << std::endl;
    // int d = DepthOfTree1(root);
    // std::cout << "d: " << d << std::endl;

    std::cout << "3. PathLeafth: " << std::endl;
    vector<string> result;
    PathLeafth(root, result);
    for (const string& path : result) {
        cout << " " << path << endl;
    }
    // ==================
 
    // clean up
    DeleteTree(root);
    return 0;
}

/*
0.
          5
      ┌───┴───┐
      1       7

1.
          5
      ┌───┴───┐
      1       7
      ┴─┐     ┴─┐
        3       8
       ┌┘
       2

2.
          5
      ┌───┴───┐
      1       7
    ┌─┴─┐   ┌─┴─┐
    6   8

3.
          5
      ┌───┴───┐
      1       7
    ┌─┴─┐   ┌─┴─┐
    6   8
      ┌─┴─┐
      2   3

4.
          5
      ┌───┴───┐
      1       7
    ┌─┴─┐   ┌─┴─┐
            6   8
*/
