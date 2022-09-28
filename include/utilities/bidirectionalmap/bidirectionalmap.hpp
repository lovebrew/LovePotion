#pragma once

#include <array>
#include <functional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

template<typename K = void, typename V = void, std::size_t Size = 0, typename KC = std::equal_to<>,
         typename VC = std::equal_to<>>
class BidirectionalMap;

template<>
class BidirectionalMap<>
{
  protected:
    BidirectionalMap() = default;

  public:
    // Used to compare C strings; operator== doesn't work properly for those
    struct cstringcomp
    {
        constexpr bool operator()(const char* a, const char* b) const
        {
            if (a == nullptr && b == nullptr)
            {
                return true;
            }
            else if (a == nullptr || b == nullptr)
            {
                return false;
            }
            return std::string_view(a) == std::string_view(b);
        }

        constexpr bool operator()(const char* a, std::string_view b) const
        {
            return a == b;
        }

        constexpr bool operator()(std::string_view b, const char* a) const
        {
            return a == b;
        }
    };

    template<typename A, typename B, typename... Args>
    struct CheckArgs
    {
      private:
        using CheckArgs2 = CheckArgs<Args...>;

        static constexpr bool KeyConvertible =
            std::is_convertible_v<std::remove_cvref_t<A>,
                                  std::remove_cvref_t<typename CheckArgs2::AType>> ||
            std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::AType>,
                                  std::remove_cvref_t<A>> ||
            std::is_convertible_v<std::remove_cvref_t<A>,
                                  std::decay_t<typename CheckArgs2::AType>> ||
            std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::AType>, std::decay_t<A>>;
        static constexpr bool ValueConvertible =
            std::is_convertible_v<std::remove_cvref_t<B>,
                                  std::remove_cvref_t<typename CheckArgs2::BType>> ||
            std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::BType>,
                                  std::remove_cvref_t<B>> ||
            std::is_convertible_v<std::remove_cvref_t<B>,
                                  std::decay_t<typename CheckArgs2::BType>> ||
            std::is_convertible_v<std::decay_t<typename CheckArgs2::BType>, std::remove_cvref_t<B>>;

      public:
        static constexpr bool value = KeyConvertible && ValueConvertible && CheckArgs2::value;

        using AType = std::conditional_t<
            std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::AType>,
                                  std::remove_cvref_t<A>>,
            std::remove_cvref_t<A>,
            std::conditional_t<
                std::is_convertible_v<std::remove_cvref_t<A>,
                                      std::remove_cvref_t<typename CheckArgs2::AType>>,
                std::remove_cvref_t<typename CheckArgs2::AType>,
                std::conditional_t<
                    std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::AType>,
                                          std::decay_t<A>>,
                    std::decay_t<A>, std::decay_t<typename CheckArgs2::AType>>>>;
        using BType = std::conditional_t<
            std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::BType>,
                                  std::remove_cvref_t<B>>,
            std::remove_cvref_t<B>,
            std::conditional_t<
                std::is_convertible_v<std::remove_cvref_t<B>,
                                      std::remove_cvref_t<typename CheckArgs2::BType>>,
                std::remove_cvref_t<typename CheckArgs2::BType>,
                std::conditional_t<
                    std::is_convertible_v<std::remove_cvref_t<typename CheckArgs2::BType>,
                                          std::decay_t<B>>,
                    std::decay_t<B>, std::decay_t<typename CheckArgs2::BType>>>>;
    };

    template<typename A, typename B>
    struct CheckArgs<A, B>
    {
        static constexpr bool value = true;
        using AType                 = std::remove_cvref_t<A>;
        using BType                 = std::remove_cvref_t<B>;
    };

    template<typename T>
    struct DefaultComparatorForType
    {
      private:
        using testtype = std::remove_cvref_t<T>;

      public:
        using value =
            std::conditional_t<std::is_same_v<testtype, const char*>, cstringcomp, std::equal_to<>>;
    };

    template<typename T>
    using defaultcomp_v = typename DefaultComparatorForType<T>::value;

    template<typename... Args>
    struct ValidArgs_s : public std::false_type
    {
    };

    // clang-format off
    template<typename... Args>
    requires (sizeof...(Args) % 2 == 0) && (sizeof...(Args) > 0)
    struct ValidArgs_s<Args...>
    // clang-format on
    {
        static constexpr bool value = CheckArgs<Args...>::value;
    };

    template<typename... Args>
    static constexpr bool ValidArgs_v = ValidArgs_s<Args...>::value;

    template<typename KC, typename VC, typename... Args>
    struct ValidComparatorArgs_s : public std::false_type
    {
    };

    // clang-format off
    template<typename KC, typename VC, typename... Args>
    requires (sizeof...(Args) % 2 == 0) && (sizeof...(Args) > 0)
    struct ValidComparatorArgs_s<KC, VC, Args...>
    // clang-format on
    {
        using Check = CheckArgs<Args...>;
        using AType = Check::AType;
        using BType = Check::BType;

        static constexpr bool value = Check::value && (requires {
                                          requires std::equivalence_relation<KC, AType, AType>;
                                          requires std::equivalence_relation<VC, BType, BType>;
                                      });
    };

    template<typename KC, typename VC, typename... Args>
    static constexpr bool ValidComparatorArgs_v = ValidComparatorArgs_s<KC, VC, Args...>::value;

  public:
    // Note: long name, but shouldn't often be used
    // clang-format off
    template<typename KeyComparator, typename ValueComparator, typename... Args>
    requires ValidComparatorArgs_v<KeyComparator, ValueComparator, Args...>
    static consteval auto CreateWithComparators(KeyComparator kc, ValueComparator vc, Args... args)
    // clang-format on
    {
        using check                = CheckArgs<Args...>;
        using AType                = typename check::AType;
        using BType                = typename check::BType;
        using PairType             = std::pair<AType, BType>;
        constexpr std::size_t Size = sizeof...(Args) / 2;

        // clang-format off
        auto setArgs = [](std::array<PairType, Size>& addTo, Args... args) {
            auto setArgsRef = []<typename... InnerArgs>(auto& me, std::array<PairType, Size>& addTo,
                                                        AType key, BType val, InnerArgs... args)
            {
                std::size_t index   = addTo.size() - (sizeof...(InnerArgs) + 2) / 2;
                addTo[index].first  = key;
                addTo[index].second = val;

                if constexpr (sizeof...(InnerArgs) != 0)
                {
                    me(me, addTo, std::forward<InnerArgs>(args)...);
                }
            };
            setArgsRef(setArgsRef, addTo, args...);
        };
        // clang-format on

        std::array<PairType, Size> entries {};

        setArgs(entries, std::forward<Args>(args)...);

        return BidirectionalMap<AType, BType, Size, KeyComparator, ValueComparator> { entries, kc,
                                                                                      vc };
    }

    // Note: long name, but shouldn't often be used
    // clang-format off
    template<typename KeyComparator = std::equal_to<>, typename... Args>
    requires ValidComparatorArgs_v<KeyComparator, defaultcomp_v<typename CheckArgs<Args...>::BType>, Args...>
    static consteval auto CreateWithKeyComparator(KeyComparator kc = KeyComparator(), Args... args)
    // clang-format on
    {
        return CreateWithComparators(std::move(kc),
                                     defaultcomp_v<typename CheckArgs<Args...>::BType>(), args...);
    }

    // Note: long name, but shouldn't often be used
    // clang-format off
    template<typename ValueComparator = std::equal_to<>, typename... Args>
    requires ValidComparatorArgs_v<defaultcomp_v<typename CheckArgs<Args...>::AType>, ValueComparator, Args...>
    static consteval auto CreateWithValueComparator(ValueComparator vc = ValueComparator(), Args... args)
    // clang-format on
    {
        return CreateWithComparators(defaultcomp_v<typename CheckArgs<Args...>::AType>(),
                                     std::move(vc), args...);
    }

    // clang-format off
    template<typename... Args>
    requires ValidArgs_v<Args...>
    static consteval auto Create(Args... args)
    // clang-format on
    {
        return CreateWithComparators(defaultcomp_v<typename CheckArgs<Args...>::AType>(),
                                     defaultcomp_v<typename CheckArgs<Args...>::BType>(), args...);
    }
};

// clang-format off
template<std::default_initializable K, std::default_initializable V, std::size_t Size,
         std::equivalence_relation<K, K> KC, std::equivalence_relation<V, V> VC>
    requires (Size > 0)
class BidirectionalMap<K, V, Size, KC, VC> : private BidirectionalMap<>
// clang-format on
{
  private:
    // clang-format off
    template<typename T, std::size_t ArraySize>
    requires(ArraySize != std::dynamic_extent)
    consteval void populate(std::span<T, ArraySize> data)
    // clang-format on
    {
        static_assert(ArraySize <= Size && ArraySize > 0);

        for (std::size_t i = 0; i < ArraySize; i++)
            this->entries[i] = data[i];

        if constexpr (ArraySize < Size)
        {
            for (std::size_t i = ArraySize; i < Size; i++)
                this->entries[i] = { Key(), Value() };
        }
    }

  public:
    using Key             = std::remove_cvref_t<K>;
    using Value           = std::remove_cvref_t<V>;
    using KeyComparator   = std::remove_cvref_t<KC>;
    using ValueComparator = std::remove_cvref_t<VC>;

    using Entry = std::pair<Key, Value>;

    using BidirectionalMap<>::CreateWithComparators;
    using BidirectionalMap<>::CreateWithKeyComparator;
    using BidirectionalMap<>::CreateWithValueComparator;

    BidirectionalMap() = delete;

    template<std::size_t ArraySize = Size>
    consteval BidirectionalMap(const std::pair<K, V> (&inEntries)[ArraySize], KC kc = KC {},
                               VC vc = VC {}) :
        entries(),
        populated(ArraySize),
        kc(kc),
        vc(vc)
    {
        populate(inEntries);
    }

    template<std::size_t ArraySize = Size>
    consteval BidirectionalMap(const std::array<std::pair<K, V>, ArraySize>(&inEntries),
                               KC kc = KC {}, VC vc = VC {}) :
        entries(),
        populated(ArraySize),
        kc(kc),
        vc(vc)
    {
        populate(inEntries);
    }

    // clang-format off
    template<typename... Args>
    requires ValidComparatorArgs_v<KC, VC, Args...>
    consteval BidirectionalMap(KC&& kc, VC&& vc, Args&&... args) :
        entries {},
        populated(Size),
        kc(kc),
        vc(vc)
    // clang-format on
    {
        auto setArgs = [](std::array<Entry, Size>& addTo, Args&&... args) {
            auto setArgsRef = []<typename A, typename B, typename... InnerArgs>(
                                  auto& me, std::array<Entry, Size>& addTo, A&& key, B&& val,
                                  InnerArgs&&... args) {
                std::size_t index   = addTo.size() - (sizeof...(InnerArgs) + 2) / 2;
                addTo[index].first  = key;
                addTo[index].second = val;

                if constexpr (sizeof...(InnerArgs) != 0)
                {
                    me(me, addTo, std::forward<InnerArgs>(args)...);
                }
            };
            setArgsRef(setArgsRef, addTo, args...);
        };

        setArgs(entries, std::forward<Args>(args)...);
    }

    // clang-format off
    template<typename... Args>
    requires ValidArgs_v<Args...>
    consteval BidirectionalMap(Args&&... args) :
        BidirectionalMap(defaultcomp_v<K>(), defaultcomp_v<V>(), std::forward<Args>(args)...)
    // clang-format on
    {}

    /*
    ** When mapped as T, V -- find value V from T
    */
    constexpr std::optional<std::reference_wrapper<const Value>> Find(const Key& search) const
    {
        for (std::size_t i = 0; i < this->populated; ++i)
        {
            if (kc(this->entries[i].first, search))
            {
                return std::ref(this->entries[i].second);
            }
        }

        return std::nullopt;
    }

    /*
    ** When mapped as T, V -- find value T from V
    */
    constexpr std::optional<std::reference_wrapper<const Key>> ReverseFind(
        const Value& search) const
    {
        for (std::size_t i = 0; i < this->populated; ++i)
        {
            if (vc(this->entries[i].second, search))
            {
                return std::ref(this->entries[i].first);
            }
        }

        return std::nullopt;
    }

    constexpr auto GetKeys() const
    {
        return std::ranges::ref_view(this->entries) | std::views::keys |
               std::views::take(this->populated);
    }

    /* Can only be used on String-mapped Keys */
    // clang-format off
    constexpr auto GetNames() const
        requires (std::is_same_v<Key, const char*> || std::is_same_v<Key, char*> ||
                  std::is_same_v<Key, std::string_view>)
    // clang-format on
    {
        return GetKeys();
    }

    constexpr auto GetValues() const
    {
        return std::ranges::ref_view(this->entries) | std::views::values |
               std::views::take(this->populated);
    }

    constexpr std::span<const Entry> GetEntries() const
    {
        return { entries.begin(), entries.begin() + populated };
    }

  private:
    std::array<Entry, Size> entries;
    const std::size_t populated;

    [[no_unique_address]] KeyComparator kc;
    [[no_unique_address]] ValueComparator vc;
};

// clang-format off
template<typename KC, typename VC, typename... Args>
requires BidirectionalMap<>::ValidComparatorArgs_v<KC, VC, Args&&...>
BidirectionalMap(KC&&, VC&&, Args&&...) -> BidirectionalMap<typename BidirectionalMap<>::CheckArgs<Args&&...>::AType,
                                                      typename BidirectionalMap<>::CheckArgs<Args&&...>::BType,
                                                      sizeof...(Args) / 2, KC, VC>;

template<typename... Args>
requires BidirectionalMap<>::ValidArgs_v<Args&&...>
BidirectionalMap(Args&&...) -> BidirectionalMap<typename BidirectionalMap<>::CheckArgs<Args&&...>::AType,
                                              typename BidirectionalMap<>::CheckArgs<Args&&...>::BType,
                                              sizeof...(Args) / 2,
                                              BidirectionalMap<>::defaultcomp_v<typename BidirectionalMap<>::CheckArgs<Args&&...>::AType>,
                                              BidirectionalMap<>::defaultcomp_v<typename BidirectionalMap<>::CheckArgs<Args&&...>::BType>>;
// clang-format on
