#include "../Pixl.h"

int main()
{
    PixlDoc *doc = PixlDoc_Load("./test.xml");

    XMLNode *node = doc->root;
    printf("Attributes: \n");
    for (int i = 0; i < node->attributes->size; i++)
    {
        XMLNodeAttr attr = node->attributes->data[i];
        printf("[ %s => %s ]\n", attr.key, attr.value);
    }
    PixlDoc_Free(doc);

    return 0;
}