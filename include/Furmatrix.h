#pragma once

// TODO: PLEASE USE BETTER STRUCTURES FOR MATH

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

    Furmatrix operator*(const Furmatrix& other) const
    {
        if (this->ncols != other.nrows) {
            throw invalid_argument("Not valid!");
        }

        Furmatrix result(this->nrows, other.ncols);

        for (int i = 0; i < this->nrows; i++) {
            for (int j = 0; j < other.ncols; j++) {
                for (int k = 0; k < this->ncols; k++) {
                    result.data[i][j] += this->data[i][k] * other.data[k][j];
                }
            }
        }
        return result;
    }

};
