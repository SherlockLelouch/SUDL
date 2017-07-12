/*
*@filname lstm.cpp
*/
#include "lstm.h"
#include <iostream>
#include <math.h>
#include <fstream>

namespace sub_dl {

template <typename T>
void gradident_clip(Matrix<T>& matrix) {
    for (size_t i = 0; i < matrix._x_dim; i++) {
        for (size_t j = 0; j < matrix._y_dim; j++) {
            if (matrix[i][j] > 5.0 || matrix[i][j] < -5.0) {
                matrix[i][j] /= 10.0;
            }
        }
    }
}

float tanh(float x) {
    return (exp(x) - exp(-x)) / (exp(x) + exp(-x));
}

float sigmoid(float x) {
    return 1.0 / (1 + exp(-x));
}

template <typename T>
Matrix<T> tanh_m(const Matrix<T>& matrix) {
    Matrix<T> ret_val(matrix._x_dim, matrix._y_dim);
    for (size_t i = 0; i < matrix._x_dim; i++) {
        for (size_t j = 0; j < matrix._y_dim; j++) {
            ret_val[i][j] = tanh(matrix[i][j]);
        }
    }
    return ret_val;
}

template <typename T>
Matrix<T> tanh_m_diff(const Matrix<T>& matrix) {
    Matrix<T> ret_val(matrix._x_dim, matrix._y_dim);
    for (size_t i = 0; i < matrix._x_dim; i++) {
        for (size_t j = 0; j < matrix._y_dim; j++) {
            ret_val[i][j] = 1 - tanh(matrix[i][j]) * tanh(matrix[i][j]);
        }
    }
    return ret_val;
}

template <typename T>
Matrix<T> sigmoid_m(const Matrix<T>& matrix) {
    Matrix<T> ret_val(matrix._x_dim, matrix._y_dim);
    for (size_t i = 0; i < matrix._x_dim; i++) {
        for (size_t j = 0; j < matrix._y_dim; j++) {
            ret_val[i][j] = sigmoid(matrix[i][j]);
        }
    }
    return ret_val;
}

template <typename T>
Matrix<T> sigmoid_m_diff(const Matrix<T>& matrix) {
    Matrix<T> ret_val(matrix._x_dim, matrix._y_dim);
    for (size_t i = 0; i < matrix._x_dim; i++) {
        for (size_t j = 0; j < matrix._y_dim; j++) {
            ret_val[i][j] = matrix[i][j] * (1 - matrix[i][j]);
        }
    }
    return ret_val;
}

int merge(const Matrix<float>& output_val) {
    int val = 0;
    int d = 1;
    for (int i = 8; i >= 1; i--) {
        val += int(output_val[i - 1][0] + 0.5) * pow(2, i - 1);
    }
    return val;
}


LSTM::LSTM() {
}

LSTM::LSTM(int feature_dim, int hidden_dim, int output_dim) : 
    _feature_dim(feature_dim), 
    _hidden_dim(hidden_dim), 
    _output_dim(output_dim) {

    _ig_input_weights.resize(feature_dim, hidden_dim);
    _ig_hidden_weights.resize(hidden_dim, hidden_dim);
    _ig_bias.resize(1, hidden_dim);

    _ig_input_weights.assign_val();
    _ig_hidden_weights.assign_val();
    _ig_bias.assign_val();

    _fg_input_weights.resize(feature_dim, hidden_dim);
    _fg_hidden_weights.resize(hidden_dim, hidden_dim);
    _fg_bias.resize(1, hidden_dim);

    _fg_input_weights.assign_val();
    _fg_hidden_weights.assign_val();
    _fg_bias.assign_val();
    
    _og_input_weights.resize(feature_dim, hidden_dim);
    _og_hidden_weights.resize(hidden_dim, hidden_dim);
    _og_bias.resize(1, hidden_dim);

    _og_input_weights.assign_val();
    _og_hidden_weights.assign_val();
    _og_bias.assign_val();

    _cell_input_weights.resize(feature_dim, hidden_dim);
    _cell_hidden_weights.resize(hidden_dim, hidden_dim);
    _cell_bias.resize(1, hidden_dim);

    _cell_input_weights.assign_val();
    _cell_hidden_weights.assign_val();
    _cell_bias.assign_val();

}

float LSTM::_forward(const std::vector<int>& sample_indexes, int epoch) {

    float cost = 0.0;
    int val1, val2;
    for (size_t i = 0; i < sample_indexes.size(); i++) {
        const matrix_float& feature = _train_x_features[sample_indexes[i]];
        const matrix_float& label = _train_y_labels[sample_indexes[i]];
        int time_step_cnt = feature._x_dim;
        _output_values.resize(time_step_cnt, _output_dim);
        _hidden_values.resize(time_step_cnt, _hidden_dim);
        _cell_values.resize(time_step_cnt, _hidden_dim);
        
        matrix_float pre_hidden_vals(1, _hidden_dim);
        matrix_float pre_cell_vals(1, _hidden_dim);
        for (size_t t = 1; t <= time_step_cnt; t++) {
            // xt
            const matrix_float& xt = feature._R(t - 1);
            // fo = sigmoid(xt * _fg_input_weights + h(t-1) * _fg_hidden_weights + bias)
            matrix_float f_output = sigmoid_m(xt * _fg_input_weights 
                + pre_hidden_vals * _fg_hidden_weights + _fg_bias);
            // io = sigmod_m(xt * _ig_input_weights + h(t - 1) * _ig_hidden_weights + bias)
            matrix_float i_output = sigmoid_m(xt * _ig_input_weights 
                + pre_hidden_vals * _ig_hidden_weights + _ig_bias);
            // fo = sigmoid(xt * _og_input_weights + h(t-1) * _og_hidden_weights + bias)
            matrix_float o_output = sigmoid_m(xt * _og_input_weights 
                + pre_hidden_vals * _og_hidden_weights + _og_bias);
            // c^~(t) = sigmoid(xt * _cell_input_weights + h(t-1) * _cell_hidden_weights) + bias
            matrix_float cell_new_val = sigmoid_m(xt * _cell_input_weights 
                + pre_hidden_vals * _cell_hidden_weights + _cell_bias);
            // ct = c(t-1) * fo + c^~(t) * io
            matrix_float cell_output = cell_new_val.dot_mul(i_output) + pre_cell_vals.dot_mul(f_output);
            pre_cell_vals = cell_output;
            pre_hidden_vals = tanh_m(cell_val).dot_mul(o_output);
            _cell_values.set_row(t - 1, pre_cell_vals);
            _hidden_values.set_row(t - 1, pre_hidden_vals);
        } 
        val1 = merge(label);
        val2 = merge(_output_values);
        float eta = -0.1;
        // calc error
        matrix_float diff_val = label - _output_values;
        cost += diff_val.dot_mul(diff_val).sum() * 0.5;
        // error back propogation
        matrix_float nxt_hidden_error(1, _hidden_dim);
        matrix_float delta_hidden_output_weights(_hidden_output_weights._x_dim, 
            _hidden_output_weights._y_dim);
        matrix_float delta_input_hidden_weights(_input_hidden_weights._x_dim,
            _input_hidden_weights._y_dim);
        matrix_float delta_hidden_weights(_hidden_weights._x_dim,
            _hidden_weights._y_dim);
        matrix_float delta_hidden_bias(_hidden_bias._x_dim, _hidden_bias._y_dim);
        matrix_float delta_output_bias(_output_bias._x_dim, _output_bias._y_dim);
        for (size_t t = time_step_cnt; t >= 1; t--) {
            // output_error = (o_t - y_t) * f'(o_t)
            matrix_float output_error = (_output_values._R(t - 1) - label._R(t - 1)) \
                .dot_mul(sigmoid_m_diff(_output_values._R(t - 1)));
            // hidden_error = 
            // (output_error * V_jk + nxt_hidden_error * U_jr) dot_multiply f'(hidden_values[t-1])
            matrix_float hidden_error = (output_error * _hidden_output_weights._T() + 
                nxt_hidden_error * _hidden_weights._T()) \
                .dot_mul(tanh_m_diff(_hidden_values._R(t - 1)));
            //_hidden_output_weights.add(_hidden_values._R(t - 1)._T() * output_error * eta);
            // delta_V = delta_V + hidden_values[t-1]^T * output_error
            delta_hidden_output_weights.add(_hidden_values._R(t - 1)._T() * 
                output_error);
            //_input_hidden_weights.add(feature._R(t - 1)._T() * hidden_error * eta);
            delta_input_hidden_weights.add(feature._R(t - 1)._T() * hidden_error);
            if (t > 1) {
                //_hidden_weights.add(_hidden_values._R(t - 2)._T() * hidden_error * eta); 
                delta_hidden_weights.add(_hidden_values._R(t - 2)._T() * hidden_error);
            }
            delta_hidden_bias.add(hidden_error);
            delta_output_bias.add(output_error);
            nxt_hidden_error = hidden_error; 
        }
        // weight update
        _hidden_output_weights.add(delta_hidden_output_weights * eta);
        _input_hidden_weights.add(delta_input_hidden_weights * eta);
        _hidden_weights.add(delta_hidden_weights * eta);
        _hidden_bias.add(delta_hidden_bias * eta);
        _output_bias.add(delta_output_bias * eta);
        gradident_clip(_hidden_output_weights);
        gradident_clip(_input_hidden_weights);
        gradident_clip(_hidden_weights);
        gradident_clip(_hidden_bias);
        gradident_clip(_output_bias);
    }
    std::cout << "Epoch " << val1 << " " << val2 << " " << cost << std::endl;
    return cost / sample_indexes.size();
}

void LSTM::_backward() {
}

void LSTM::_load_feature_data() {
    srand( (unsigned)time( NULL ) );
    // load data
    std::ifstream fis("data");
    for (size_t i = 0; i < 12500; i++) {    
        matrix_float x(8, _feature_dim);
        matrix_float y(8, _output_dim);
        int sum = 0;
        for (size_t j = 0; j < _feature_dim; j ++) {
            int v = rand() % 128;
            //int v;
            //fis >> v;
            sum += v;
            size_t k = 0;
            while (v > 0) {
                x[k][j] = v % 2;
                v /= 2;
                k += 1;
            }
            for (; k < 8;k ++) {
                x[k][j] = 0;
            }
        }
        //fis >> sum;
        size_t k = 0;
        while (sum > 0) {
            y[k][0] = sum % 2;
            sum /= 2;
            k += 1;
        }
        for (; k < 8;k ++) {
            y[k][0] = 0;
        }
        if (i < 10000) {
            _train_x_features.push_back(x);
            _train_y_labels.push_back(y);
        } else {
            _test_x_features.push_back(x);
            _test_y_labels.push_back(y);
        }
    }

}

void LSTM::_train() {

    // load x
    _load_feature_data();
    _max_epoch_cnt = 10;
    int batch_size = 100;
    for (size_t epoch = 0; epoch < _max_epoch_cnt; epoch++) {
        for (int i = 0; i < 100; i++) {
            std::vector<int> sample_indexes;
            for (int j = i * batch_size; j < (i + 1) * batch_size; j++) {
                sample_indexes.push_back(j);
            }
            _forward(sample_indexes, i);
        }
    }
}

}

using namespace sub_dl;

int main() {
    LSTM *lstm = new LSTM(2, 16, 1);
    lstm->_set_epoch_cnt(100);
    lstm->_load_feature_data();
    lstm->_train();
}
