/** \file Util.h
 * Utility definitions
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_UTIL_H
#define OPEN_SEA_UTIL_H

#include <functional>

namespace open_sea::util {
    //----- Start Class Member Utility Templates
    /**
     *  Get type of Nth member of T
     *
     * \tparam T Container type
     * \tparam N Member index
     */
    template<typename T, size_t N>
    struct GetMemberType;

    /**
     * Get type of pointer to Nth member of T
     *
     * \tparam T Container type
     * \tparam N Member index
     */
    template<typename T, size_t N>
    struct GetMemberPointerType {
        typedef typename GetMemberType<T, N>::type T::*type;
    };

    /**
     * Get pointer to Nth member of T
     *
     * \tparam T Container type
     * \tparam N Member index
     * \return Pointer to Nth member of T
     */
    template<typename T, size_t N>
    typename GetMemberPointerType<T, N>::type get_pointer_to_member();
    //----- End Class Member Utility Templates

    //----- Start Utility Type Templates
    //! Structure template giving a unique type for each N, used for type-based dispatching
    template <int N>
    struct IntToType {};
    //----- End Utility Type Templates

    //----- Start Function Utility Templates
    //! Continue type for type-based dispatching of \ref invoke_n_body
    typedef IntToType<true> invoke_continue;
    //! Stop type for type-based dispatching of \ref invoke_n_body
    typedef IntToType<false> invoke_stop;

    /**
     * \brief Invoke the functor F<I> forwarding it the provided arguments, and continue invocation loop
     *
     * The loop is continued as long as I < L.
     *
     * \tparam I Invocation counter
     * \tparam L Invocation limit
     * \tparam F Functor type
     * \tparam Args Arguments type
     * \param args Arguments
     */
    template<size_t I, size_t L, template <size_t> typename F, class... Args>
    void invoke_n_body(invoke_continue, Args &&... args) {
        // Invoke function
        std::invoke(F<I>(), std::forward<Args>(args)...);

        // Continue while I < L
        invoke_n_body<I + 1, L, F>(IntToType<((I + 1) < L)>(), std::forward<Args>(args)...);
    }

    /**
     * \brief Terminating overload of repeated functor invocation
     *
     * Invoked when I = L, and thus the desired number of functor invocations was reached.
     *
     * \tparam I Invocation counter
     * \tparam L Invocation limit
     * \tparam F Functor type
     * \tparam Args Arguments type
     */
    template<size_t I, size_t L, template <size_t> typename F, class... Args>
    void invoke_n_body(invoke_stop, Args &&... /*args*/) {}

    /**
     * Invoke the functor F<X> N times with X from 0 to N-1, forwarding the provided arguments to it
     *
     * \tparam N Number of invocations
     * \tparam F Functor type
     * \tparam Args Arguments type
     * \param args Arguments
     */
    template<size_t N, template <size_t> typename F, class... Args>
    void invoke_n(Args &&... args) {
        invoke_n_body<0, N, F>(IntToType<(0 < N)>(), std::forward<Args>(args)...);
    }
    //----- End Function Utility Templates
}

#endif //OPEN_SEA_UTIL_H
