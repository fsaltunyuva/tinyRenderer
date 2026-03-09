#pragma once

using namespace std;

class Furmatrix
{
public:
    int nrows, ncols;
    vector<vector<float>> data;

    Furmatrix(int nrows, int ncols)
    {
        this->nrows = nrows;
        this->ncols = ncols;
        data.assign(nrows, vector<float>(ncols, 0.f));
    }

    void addData(vector<float> newData)
    {
        // For now, only supports 3x3
        for (int i = 0; i < nrows; i++)
            for (int j = 0; j < ncols; j++)
                data[i][j] = newData[i * ncols + j];
    }

};
