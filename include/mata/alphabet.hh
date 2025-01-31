/* alphabet.hh -- File containing alphabets for automata.
 */

#ifndef MATA_ALPHABET_HH
#define MATA_ALPHABET_HH

#include <forward_list>
#include <string>
#include <utility>

#include "utils/utils.hh"
#include "utils/ord-vector.hh"

namespace mata {

using Symbol = unsigned;
using Word = std::vector<Symbol>;
using WordName = std::vector<std::string>;

 /**
  * The abstract interface for NFA alphabets.
  */
class Alphabet {
public:
    /// translates a string into a symbol
    virtual Symbol translate_symb(const std::string &symb) = 0;

    /**
     * Translate sequence of symbol names to sequence of their respective values.
     */
    virtual Word translate_word(const WordName& word_name) const {
        (void)word_name;
        throw std::runtime_error("Unimplemented");
    }

    /**
     * @brief Translate internal @p symbol representation back to its original string name.
     *
     * Throws an exception when the @p symbol is missing in the alphabet.
     * @param[in] symbol Symbol to translate.
     * @return @p symbol original name.
     */
    virtual std::string reverse_translate_symbol(Symbol symbol) const = 0;

    /// also translates strings to symbols
    Symbol operator[](const std::string &symb) { return this->translate_symb(symb); }

    /**
     * @brief Get a set of all symbols in the alphabet.
     *
     * The result does not have to equal the list of symbols in the automaton using this alphabet.
     */
    virtual utils::OrdVector<Symbol> get_alphabet_symbols() const { throw std::runtime_error("Unimplemented"); }

    /// complement of a set of symbols wrt the alphabet
    virtual utils::OrdVector<Symbol> get_complement(const utils::OrdVector<Symbol>& symbols) const { // {{{
        (void) symbols;
        throw std::runtime_error("Unimplemented");
    } // }}}

    virtual ~Alphabet() = default;

    /**
     * @brief Check whether two alphabets are equal.
     *
     * In general, two alphabets are equal if and only if they are of the same class instance.
     * @param other_alphabet The other alphabet to compare with for equality.
     * @return True if equal, false otherwise.
     */
    virtual bool is_equal(const Alphabet &other_alphabet) const { return address() == other_alphabet.address(); }

    /**
     * @brief Check whether two alphabets are equal.
     *
     * In general, two alphabets are equal if and only if they are of the same class instance.
     * @param other_alphabet The other alphabet to compare with for equality.
     * @return True if equal, false otherwise.
     */
    virtual bool is_equal(const Alphabet *const other_alphabet) const {
        return address() == other_alphabet->address();
    }

    bool operator==(const Alphabet &) const = delete;

    /**
     * Checks whether the alphabet has any symbols.
     */
    virtual bool empty() const = 0;

    virtual void clear() { throw std::runtime_error("Unimplemented"); }

protected:
    virtual const void* address() const { return this; }
}; // class Alphabet.

/**
* Direct alphabet (also identity alphabet or integer alphabet) using integers as symbols.
*
* This alphabet presumes that all integers are valid symbols.
* Therefore, calling member functions get_complement() and get_alphabet_symbols() makes no sense in this context and the methods
*  will throw exceptions warning about the inappropriate use of IntAlphabet. If one needs these functions, they should
*  use OnTheFlyAlphabet instead of IntAlphabet.
*/
class IntAlphabet : public Alphabet {
public:
    IntAlphabet() : alphabet_instance(IntAlphabetSingleton::get()) {}

    Symbol translate_symb(const std::string &symb) override;

    std::string reverse_translate_symbol(Symbol symbol) const override { return std::to_string(symbol); }

    utils::OrdVector<Symbol> get_alphabet_symbols() const override {
        throw std::runtime_error("Nonsensical use of get_alphabet_symbols() on IntAlphabet.");
    }

    utils::OrdVector<Symbol> get_complement(const utils::OrdVector<Symbol>& symbols) const override {
        (void) symbols;
        throw std::runtime_error("Nonsensical use of get_complement() on IntAlphabet.");
    }

    IntAlphabet(const IntAlphabet&) = default;

    IntAlphabet& operator=(const IntAlphabet& int_alphabet) = delete;

    bool empty() const override { return false; }

    void clear() override { throw std::runtime_error("Nonsensical use of clear() on IntAlphabet."); }

protected:
    const void* address() const override { return &alphabet_instance; }

private:
    /**
     * Singleton class implementing integer alphabet_instance for class IntAlphabet.
     *
     * Users have to use IntAlphabet instead which provides interface identical to other alphabets and can be used in
     *  places where an instance of the abstract class Alphabet is required.
     */
    class IntAlphabetSingleton {
    public:
        static IntAlphabetSingleton& get() {
            static IntAlphabetSingleton alphabet;
            return alphabet;
        }

        IntAlphabetSingleton(IntAlphabetSingleton&) = delete;
        IntAlphabetSingleton(IntAlphabetSingleton&&) = delete;
        IntAlphabetSingleton& operator=(const IntAlphabetSingleton&) = delete;
        IntAlphabetSingleton& operator=(IntAlphabetSingleton&&) = delete;

        ~IntAlphabetSingleton() = default;

    protected:
        IntAlphabetSingleton() = default;
    }; // class IntAlphabetSingleton.

    IntAlphabetSingleton& alphabet_instance;
}; // class IntAlphabet.

/**
 * Enumerated alphabet using a set of integers as symbols maintaining a set of specified symbols.
 *
 * @c EnumAlphabet is a version of direct (identity) alphabet (does not give names to symbols, their name is their
 *  integer value directly). However, unlike @c IntAlphabet, @c EnumAlphabet maintains an ordered set of symbols in
 *  the alphabet.
 *
 * Therefore, calling member functions @c get_complement() and @c get_alphabet_symbols() makes sense in the context
 *  of @c EnumAlphabet and the functions give the expected results.
 *
 *  Example:
 *  ```cpp
 *  Alphabet alph{ EnumAlphabet{ 0, 4, 6, 8, 9 } };
 *  CHECK(alph.translate_symb("6") == 6);
 *  CHECK_THROWS(alph.translate_symb("5")); // Throws an exception about an unknown symbol.
 *  CHECK(alph.get_complement({ utils::OrdVector<Symbol>{ 0, 6, 9 } }) == utils::OrdVector<Symbol>{ 4, 8 });
 *  ```
 */
class EnumAlphabet : public Alphabet {
public:
    explicit EnumAlphabet() = default;
    EnumAlphabet(const EnumAlphabet& alphabet) = default;
    explicit EnumAlphabet(const EnumAlphabet* const alphabet): EnumAlphabet(*alphabet) {}
    EnumAlphabet(EnumAlphabet&& rhs) = default;

    utils::OrdVector<Symbol> get_alphabet_symbols() const override { return symbols_; }
    utils::OrdVector<Symbol> get_complement(const utils::OrdVector<Symbol>& symbols) const override {
        return symbols_.difference(symbols);
    }

    std::string reverse_translate_symbol(Symbol symbol) const override;

    EnumAlphabet& operator=(const EnumAlphabet& rhs) = default;
    EnumAlphabet& operator=(EnumAlphabet&& rhs) = default;

    /**
     * @brief Expand alphabet by symbols from the passed @p symbols.
     *
     * Adding a symbol name which already exists will throw an exception.
     * @param[in] symbols Vector of symbols to add.
     */
    void add_symbols_from(const mata::utils::OrdVector<Symbol>& symbols) { symbols_.insert(symbols); }

    /**
     * @brief Expand alphabet by symbols from the passed @p alphabet.
     *
     * @param[in] symbols_to_add Vector of symbols to add.
     */
    void add_symbols_from(const EnumAlphabet& alphabet) { symbols_.insert(alphabet.get_alphabet_symbols()); }

    EnumAlphabet(std::initializer_list<Symbol> symbols) : EnumAlphabet(symbols.begin(), symbols.end()) {}
    template <class InputIt> EnumAlphabet(InputIt first, InputIt last) : EnumAlphabet() {
        for (; first != last; ++first) { add_new_symbol(*first); }
    }
    EnumAlphabet(std::initializer_list<std::string> l) : EnumAlphabet(l.begin(), l.end()) {}

    Symbol translate_symb(const std::string& str) override;
    Word translate_word(const WordName& word_name) const override;

    /**
     * @brief Add new symbol to the alphabet with the value identical to its string representation.
     *
     * @param[in] symbol User-space representation of the symbol.
     * @return Result of the insertion as @c InsertionResult.
     */
    void add_new_symbol(const std::string& symbol);

    /**
     * @brief Add new symbol to the alphabet.
     *
     * @param[in] key User-space representation of the symbol.
     * @param[in] symbol Number of the symbol to be used on transitions.
     * @return Result of the insertion as @c InsertionResult.
     */
    void add_new_symbol(Symbol symbol);

    /**
     * Get the next value for a potential new symbol.
     * @return Next Symbol value.
     */
    Symbol get_next_value() const { return next_symbol_value_; }

    /**
     * Get the number of existing symbols, epsilon symbols excluded.
     * @return The number of symbols.
     */
    size_t get_number_of_symbols() const { return symbols_.size(); }

    bool empty() const override { return symbols_.empty(); }

private:
    mata::utils::OrdVector<Symbol> symbols_{}; ///< Map of string transition symbols to symbol values.
    Symbol next_symbol_value_{ 0 }; ///< Next value to be used for a newly added symbol.

public:
    /**
     * @brief Update next symbol value when appropriate.
     *
     * When the newly inserted value is larger or equal to the current next symbol value, update the next symbol
     *  value to a value one larger than the new value.
     * @param value The value of the newly added symbol.
     */
    void update_next_symbol_value(Symbol value);

    /**
     * @brief Erase a symbol from the alphabet.
     * @return Number of symbols erased (0 or 1).
     */
    size_t erase(const Symbol symbol);

    /**
     * @brief Remove a symbol name value pair from the position @p pos from the alphabet.
     * @return Iterator following the last removed element.
     */
    void erase(utils::OrdVector<Symbol>::const_iterator pos) { symbols_.erase(pos); }

    /**
     * @brief Remove a symbol name value pair from the positions between @p first and @p last from the alphabet.
     * @return Iterator following the last removed element.
     */
    void erase(utils::OrdVector<Symbol>::const_iterator first, utils::OrdVector<Symbol>::const_iterator last) {
        symbols_.erase(first, last);
    }

    void clear() override { symbols_.clear(); next_symbol_value_ = 0; }
}; // class EnumAlphabet.

/**
 * An alphabet constructed 'on the fly'.
 * Should be use anytime the automata have a specific names for the symbols.
 */
class OnTheFlyAlphabet : public Alphabet {
public:
    using StringToSymbolMap = std::unordered_map<std::string, Symbol>;

    /// Result of the insertion of a new symbol.
    using InsertionResult = std::pair<StringToSymbolMap::const_iterator, bool>;

    explicit OnTheFlyAlphabet(Symbol init_symbol = 0) : next_symbol_value_(init_symbol) {};
    OnTheFlyAlphabet(const OnTheFlyAlphabet& alphabet) = default;
    OnTheFlyAlphabet(OnTheFlyAlphabet&& alphabet) = default;
    explicit OnTheFlyAlphabet(const OnTheFlyAlphabet* const alphabet): OnTheFlyAlphabet(*alphabet) {}
    explicit OnTheFlyAlphabet(StringToSymbolMap str_sym_map) : symbol_map_(std::move(str_sym_map)) {}

    /**
     * Create alphabet from a list of symbol names.
     * @param symbol_names Names for symbols on transitions.
     * @param init_symbol Start of a sequence of values to use for new symbols.
     */
    explicit OnTheFlyAlphabet(const std::vector<std::string>& symbol_names, Symbol init_symbol = 0)
            : symbol_map_(), next_symbol_value_(init_symbol) { add_symbols_from(symbol_names); }

    template <class InputIt> OnTheFlyAlphabet(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            add_new_symbol(*first, next_symbol_value_);
        }
    }
    OnTheFlyAlphabet(std::initializer_list<std::pair<std::string, Symbol>> name_symbol_map) : symbol_map_{} {
        for (auto&& [name, symbol]: name_symbol_map) {
            add_new_symbol(name, symbol);
        }
    }

    utils::OrdVector<Symbol> get_alphabet_symbols() const override;
    utils::OrdVector<Symbol> get_complement(const utils::OrdVector<Symbol>& symbols) const override;

    std::string reverse_translate_symbol(Symbol symbol) const override;

public:
    OnTheFlyAlphabet& operator=(const OnTheFlyAlphabet& rhs) = default;
    OnTheFlyAlphabet& operator=(OnTheFlyAlphabet&& rhs) = default;

    /**
     * @brief Expand alphabet by symbols from the passed @p symbol_names.
     *
     * Adding a symbol name which already exists will throw an exception.
     * @param[in] symbol_names Vector of symbol names.
     */
    void add_symbols_from(const std::vector<std::string>& symbol_names);

    /**
     * @brief Expand alphabet by symbols from the passed @p symbol_map.
     *
     * The value of the already existing symbols will NOT be overwritten.
     * @param[in] new_symbol_map Map of strings to symbols.
     */
    void add_symbols_from(const StringToSymbolMap& new_symbol_map);

    Symbol translate_symb(const std::string& str) override;

    virtual Word translate_word(const WordName& word_name) const override;

    /**
     * @brief Add new symbol to the alphabet with the value of @c next_symbol_value.
     *
     * Throws an exception when the adding fails.
     *
     * @param[in] key User-space representation of the symbol.
     * @return Result of the insertion as @c InsertionResult.
     */
    InsertionResult add_new_symbol(const std::string& key);

    /**
     * @brief Add new symbol to the alphabet.
     *
     * Throws an exception when the adding fails.
     *
     * @param[in] key User-space representation of the symbol.
     * @param[in] value Number of the symbol to be used on transitions.
     * @return Result of the insertion as @c InsertionResult.
     */
    InsertionResult add_new_symbol(const std::string& key, Symbol value);

    /**
     * @brief Try to add symbol to the alphabet map.
     *
     * Does not throw an exception when the adding fails.
     *
     * @param[in] key User-space representation of the symbol.
     * @param[in] value Number of the symbol to be used on transitions.
     * @return Result of the insertion as @c InsertionResult.
     */
    InsertionResult try_add_new_symbol(const std::string& key, Symbol value) { return symbol_map_.insert({ key, value}); }

    /**
     * Get the next value for a potential new symbol.
     * @return Next Symbol value.
     */
    Symbol get_next_value() const { return next_symbol_value_; }

    /**
     * Get the number of existing symbols, epsilon symbols excluded.
     * @return The number of symbols.
     */
    size_t get_number_of_symbols() const { return next_symbol_value_; }

    /**
     * Get the symbol map used in the alphabet.
     * @return Map mapping strings to symbols used internally in Mata.
     */
    const StringToSymbolMap& get_symbol_map() const { return symbol_map_; }

    bool empty() const override { return symbol_map_.empty(); }

private:
    StringToSymbolMap symbol_map_{}; ///< Map of string transition symbols to symbol values.
    Symbol next_symbol_value_{}; ///< Next value to be used for a newly added symbol.

public:
    /**
     * @brief Update next symbol value when appropriate.
     *
     * When the newly inserted value is larger or equal to the current next symbol value, update the next symbol
     *  value to a value one larger than the new value.
     * @param value The value of the newly added symbol.
     */
    void update_next_symbol_value(Symbol value);

    /**
     * @brief Remove a symbol name value pair specified by its @p symbol from the alphabet.
     *
     * @warning Complexity: O(n), where n is the number of symbols in the alphabet.
     * @return Number of symbols removed (0 or 1).
     */
    size_t erase(Symbol symbol);

    /**
     * @brief Remove a symbol name value pair specified by its @p symbol_name from the alphabet.
     * @return Number of symbols removed (0 or 1).
     */
    size_t erase(const std::string& symbol_name);

    /**
     * @brief Remove a symbol name value pair from the position @p pos from the alphabet.
     */
    void erase(StringToSymbolMap::const_iterator pos) { symbol_map_.erase(pos); }

    /**
     * @brief Remove a symbol name value pair from the positions between @p first and @p last from the alphabet.
     */
    void erase(StringToSymbolMap::const_iterator first, StringToSymbolMap::const_iterator last) {
        symbol_map_.erase(first, last);
    }

    void clear() override { symbol_map_.clear(); next_symbol_value_ = 0; }
}; // class OnTheFlyAlphabet.

/**
 * @brief Encode a word using UTF-8 encoding.
 *
 * @param[in] word The word to encode.
 * @return The UTF-8 encoded word.
 */
Word encode_word_utf8(const Word& word);

/**
 * @brief Decode a word using UTF-8 encoding.
 *
 * @param[in] word The word to decode.
 * @return The UTF-8 decoded word.
 */
Word decode_word_utf8(const Word& word);

} // namespace mata

namespace std
{ // {{{
    std::ostream& operator<<(std::ostream& os, const mata::Alphabet& alphabet);
}
#endif //MATA_ALPHABET_HH
