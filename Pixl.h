#ifndef PIXL_H
#define PIXL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RESET_LEXER \
    _lexi = 0;      \
    _i++;

#define RESET_LEXER_C \
    _lexi = 0;        \
    _i++;             \
    continue;

enum
{
    T_OBR,
    T_CBR,
    T_FSL,
    T_SPC,
    T_EQ,
    T_QUOTE,
};
char __tokens__[] = {
    '<',
    '>',
    '/',
    ' ',
    '=',
    '"'};
#define _T(idx) (char)__tokens__[idx]

typedef struct _XML_Node_Attr_State
{
    char *key;
    char *value;
} XMLNodeAttr;

void XMLNodeAttr_Free(XMLNodeAttr *attr);

typedef struct _XML_Node_Attr_List
{
    size_t heapSize;
    size_t size;
    XMLNodeAttr *data;
} XMLNodeAttrList;

void XMLNodeAttrList_Init(XMLNodeAttrList *list);
void XMLNodeAttrList_Add(XMLNodeAttrList *list, XMLNodeAttr *attr);

typedef struct _XML_Node_State
{
    char *tag;
    char *content;
    struct _XML_Node_State *parent;
    struct _XML_Node_Attr_List *attributes;

} XMLNode;

XMLNode *XMLNode_New(XMLNode *parent);
void XMLNode_Free(XMLNode *node);

typedef struct _PixlDoc_State
{
    struct _XML_Node_State *root;
    char *source;
} PixlDoc;

PixlDoc *PixlDoc_Load(const char *path);
void PixlDoc_Free(PixlDoc *doc);
void PixlDoc_Parse(PixlDoc *doc, const char *buffer);

// Definations

void XMLNodeAttr_Free(XMLNodeAttr *attr)
{
    free(attr->key);
    free(attr->value);
}

void XMLNodeAttrList_Init(XMLNodeAttrList *list)
{
    list->size = 0;
    list->heapSize = 1;
    list->data = calloc(sizeof(struct _XML_Node_Attr_State), list->heapSize);
}

void XMLNodeAttrList_Add(XMLNodeAttrList *list, XMLNodeAttr *attr)
{
    while (list->size >= list->heapSize)
    {
        list->heapSize *= 2;
        list->data = (XMLNodeAttr *)realloc(list->data, sizeof(struct _XML_Node_Attr_State) * list->heapSize);
    }

    list->data[list->size++] = *attr;
}

XMLNode *XMLNode_New(XMLNode *parent)
{
    XMLNode *node = calloc(1, sizeof(struct _XML_Node_State));
    node->parent = parent;
    node->tag = (void *)0;
    node->content = (void *)0; // Content Inside a Tag
    XMLNodeAttrList_Init(node->attributes);
    return node;
}

void XMLNode_Free(XMLNode *node)
{
    if (node->tag)
        free(node->tag);
    if (node->content)
        free(node->content);
    for (int i = 0; i < node->attributes->size; i++)
        XMLNodeAttr_Free(&node->attributes->data[i]);
    free(node);
}

PixlDoc *PixlDoc_Load(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "Error Loading XML document from '%s'\n", path);
        exit(1);
    }

    PixlDoc *doc = calloc(1, sizeof(struct _PixlDoc_State));

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(sizeof(char) * size * 1);
    fread(buffer, 1, size, file);
    fclose(file);
    buffer[size] = '\0';

    doc->root = XMLNode_New((void *)0);
    PixlDoc_Parse(doc, buffer);
    return doc;
}

void PixlDoc_Parse(PixlDoc *doc, const char *buffer)
{
    char lex[256];
    int _lexi = 0, _i = 0;
    XMLNode *curNode = (void *)0;

    while (buffer[_i] != '\0')
    {
        if (buffer[_i] == _T(T_OBR))
        {
            lex[_lexi] = '\0';

            // Tag Content [ <tag> content </tag>  ]
            if (_lexi > 0)
            {
                if (!curNode)
                {
                    fprintf(stderr, "Text outside of document\n");
                    exit(1);
                }

                curNode->content = strdup(lex);
                _lexi = 0;
            }

            // End of Node [Closing Tag]
            if (buffer[_i + 1] == _T(T_FSL))
            {
                _i += 2;
                while (buffer[_i] != _T(T_CBR))
                    lex[_lexi++] = buffer[_i++];
                lex[_lexi] = '\0';

                if (!curNode)
                {
                    fprintf(stderr, "Invalid XML document\n");
                    exit(0);
                }

                if (strcmp(curNode->tag, lex))
                {
                    fprintf(stderr, "Mismatched Tags ( %s != %s )\n", curNode->tag, lex);
                    exit(1);
                }

                curNode = curNode->parent;
                _i++;
                continue;
            }

            // Create a node to save current tag
            if (!curNode)
                curNode = doc->root;
            else
                curNode = XMLNode_New(curNode);

            // Get the tag name [Begining Tag]
            _i++;

            XMLNodeAttr *curAttr;
            curAttr->key = 0;
            curAttr->value = 0;

            while (buffer[_i] != _T(T_CBR))
            {
                lex[_lexi++] = buffer[_i++];

                // Check if Attrs are Present
                if (buffer[_i] == _T(T_SPC) && !curNode->tag)
                {
                    // Save the tag name
                    lex[_lexi] = '\0';
                    curNode->tag = strdup(lex);
                    RESET_LEXER
                }

                // Ignore Rest of whitespace
                if (lex[_lexi - 1] == _T(T_SPC))
                {
                    _lexi--;
                    continue;
                }

                // Get the attribute key
                if (buffer[_i] == _T(T_EQ))
                {
                    lex[_lexi] = '\0';
                    curAttr->key = strdup(lex);
                    _lexi = 0;
                    continue;
                }

                // Get the attribute value
                if (buffer[_i] == _T(T_QUOTE))
                {
                    if (!curAttr->key)
                    {
                        fprintf(stderr, "Error parsing attributes of '%s' tag\n", curNode->tag);
                        exit(1);
                    }

                    RESET_LEXER

                    while (buffer[_i] != _T(T_QUOTE))
                        lex[_lexi++] = buffer[_i++];
                    lex[_lexi] = '\0';
                    curAttr->value = strdup(lex);
                    XMLNodeAttrList_Add(curNode->attributes, curAttr);
                    curAttr->key = (void *)0;
                    curAttr->value = (void *)0;
                    RESET_LEXER_C
                }
            }

            // If No Attr present, set the tag name
            lex[_lexi] = '\0';
            if (!curNode->tag)
                curNode->tag = strdup(lex);

            RESET_LEXER_C
        }
        else
        {
            lex[_lexi++] = buffer[_i++];
        }
    }
}

void PixlDoc_Free(PixlDoc *doc)
{
}

#endif