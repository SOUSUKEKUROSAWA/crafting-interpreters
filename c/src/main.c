#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 双方向連結リストのノード構造体
typedef struct Node {
    char* data;          // 文字列データ
    struct Node* prev;   // 前のノードへのポインタ
    struct Node* next;   // 次のノードへのポインタ
} Node;

// リストの先頭を管理する構造体
typedef struct {
    Node* head;          // リストの先頭
    Node* tail;          // リストの末尾
} List;

// どことも接続されていない新しいノードを作成する
Node* create_node(const char* str) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        return NULL;    // メモリ割り当て失敗
    }
    
    new_node->data = strdup(str);  // 文字列の複製
    if (new_node->data == NULL) {
        free(new_node);
        return NULL;    // 文字列のメモリ割り当て失敗
    }

    new_node->prev = NULL;
    new_node->next = NULL;
    return new_node;
}

// リストに新しい要素を末尾に挿入
int insert_node(List* list, const char* str) {
    if (list == NULL || str == NULL) {
        return -1;      // 無効な引数
    }

    Node* new_node = create_node(str);
    if (new_node == NULL) {
        return -1;      // ノードの作成失敗
    }

    if (list->head == NULL) {
        // リストが空の場合
        list->head = new_node;
        list->tail = new_node;
    } else {
        // 末尾に連結
        new_node->prev = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }
    return 0;
}

// リストから文字列を検索（見つかった場合はそのノードへのポインタを返す）
Node* search_node(List* list, const char* str) {
    if (list == NULL || str == NULL) {
        return NULL;
    }

    Node* current = list->head;
    while (current != NULL) {
        if (strcmp(current->data, str) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// リストからノードを削除
int delete_node(List* list, const char* str) {
    if (list == NULL || str == NULL) {
        return -1;
    }

    Node* target = search_node(list, str);
    if (target == NULL) {
        return -1;      // 要素が見つからない
    }

    // 前後のノードの接続を更新
    if (target->prev != NULL) {
        target->prev->next = target->next;
    } else {
        list->head = target->next;
    }

    if (target->next != NULL) {
        target->next->prev = target->prev;
    } else {
        list->tail = target->prev;
    }

    // メモリの解放
    free(target->data);
    free(target);
    return 0;
}

// リストの解放（メモリリーク防止）
void free_list(List* list) {
    if (list == NULL) {
        return;
    }

    Node* current = list->head;
    while (current != NULL) {
        Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
}

int main(void) {
    // リストの初期化
    List list = {NULL, NULL};

    // 要素の挿入
    insert_node(&list, "Hello");
    insert_node(&list, "World");
    insert_node(&list, "!");
    printf("Inserted: %s %s %s\n", list.head->data, list.head->next->data, list.tail->data);

    // 要素の検索
    Node* found = search_node(&list, "World");
    if (found != NULL) {
        printf("Found: %s\n", found->data);
    }

    // 要素の削除
    char* target = "World";
    delete_node(&list, target);
    printf("%s Deleted: ", target);
    Node* current = list.head;
    while (current != NULL) {
        printf("%s ", current->data);
        current = current->next;
    }
    printf("\n");

    // メモリの解放
    free_list(&list);
    return 0;
}
