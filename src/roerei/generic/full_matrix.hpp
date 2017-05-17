#pragma once

#include <roerei/generic/common.hpp>
#include <roerei/generic/encapsulated_vector.hpp>

#include <boost/optional.hpp>

#include <vector>
#include <map>

namespace roerei
{

template<typename M, typename N, typename T>
class full_matrix_t
{
public:
    typedef M row_key_t;
    typedef N column_key_t;

    typedef std::vector<T> buffer_t;

    template<typename MATRIX, typename ITERATOR>
    class row_proxy_base_t
    {
    public:
        typedef ITERATOR iterator;
        typedef MATRIX parent_t;

    private:
        friend MATRIX;

        MATRIX& parent;

    public:
        M const row_i;

    private:
        row_proxy_base_t(MATRIX& _parent, M const _row_i)
            : parent(_parent)
            , row_i(_row_i)
        {}

    public:
        T& operator[](N j)
        {
            assert(j < parent.n);
			return parent.data[row_i.unseal() * parent.n + j.unseal()];
        }

        T const operator[](N j) const
        {
            assert(j < parent.n);
			return parent.data[row_i.unseal() * parent.n + j.unseal()];
        }

        iterator begin() const
        {
			return parent.data.begin() + (row_i.unseal() * parent.n);
        }

        iterator end() const
        {
			return parent.data.begin() + ((row_i.unseal() + 1) * parent.n);
        }

        size_t size() const
        {
            return parent.n;
        }

        size_t nonempty_size() const
        {
            return parent.n;
        }
    };

    typedef row_proxy_base_t<full_matrix_t<M, N, T>, typename buffer_t::iterator> row_proxy_t;
    typedef row_proxy_base_t<full_matrix_t<M, N, T> const, typename buffer_t::const_iterator> const_row_proxy_t;

private:
    size_t m, n;
    buffer_t data;

public:
    full_matrix_t(full_matrix_t&&) = default;
    full_matrix_t(full_matrix_t&) = delete;

    full_matrix_t(size_t _m, size_t _n)
        : m(_m)
        , n(_n)
        , data(m*n)
    {}

    row_proxy_t operator[](M i)
    {
        assert(i < m);
        return row_proxy_t(*this, i);
    }

    const_row_proxy_t const operator[](M i) const
    {
        assert(i < m);
        return const_row_proxy_t(*this, i);
    }

    size_t size_m() const
    {
        return m;
    }

    size_t size_n() const
    {
        return n;
    }

    template<typename F>
    void iterate(F const& f)
    {
        for(size_t i = 0; i < m; ++i)
            f(row_proxy_t(*this, M(i)));
    }

    template<typename F>
    void citerate(F const& f) const
    {
        for(size_t i = 0; i < m; ++i)
            f(const_row_proxy_t(*this, M(i)));
    }

    template<typename F>
    void iterate(F const& f) const
    {
        citerate(f);
    }
};

}
