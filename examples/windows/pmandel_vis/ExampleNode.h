#pragma once

class CExampleNode
{
public:
    CExampleNode(void);
    ~CExampleNode(void);

    double xmin, ymin, xmax, ymax;
    int max_iter;
    CExampleNode *next;
};
