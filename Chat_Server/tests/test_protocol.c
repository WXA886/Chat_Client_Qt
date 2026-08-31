#include "../include/protocol.h"
#include <assert.h>

void test_extract_message(void)
{
    recv_buffer_t buf;
    init_recv_buffer(&buf);

    // 模拟收到两条消息
    const char *msg1 = "Hello";
    const char *msg2 = "World";

    uint32_t len1 = htonl(strlen(msg1));
    uint32_t len2 = htonl(strlen(msg2));

    // 拼装数据: [len1][msg1][len2][msg2]
    memcpy(buf.buffer, &len1, 4);
    memcpy(buf.buffer + 4, msg1, strlen(msg1));
    memcpy(buf.buffer + 4 + strlen(msg1), &len2, 4);
    memcpy(buf.buffer + 4 + strlen(msg1) + 4, msg2, strlen(msg2));
    buf.data_len = 4 + strlen(msg1) + 4 + strlen(msg2);

    // 提取第一条
    char *extracted = extract_message(&buf);
    assert(extracted != NULL);
    assert(strcmp(extracted, msg1) == 0);
    free(extracted);

    // 提取第二条
    extracted = extract_message(&buf);
    assert(extracted != NULL);
    assert(strcmp(extracted, msg2) == 0);
    free(extracted);

    printf("✅ test_extract_message 通过\n");
}

int main(void)
{
    test_extract_message();
    printf("所有测试通过！\n");
    return 0;
}