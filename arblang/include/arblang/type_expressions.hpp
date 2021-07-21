#include <string>
#include <vector>

namespace al {
namespace types {

struct quantity_type;
struct quantity_product_type;
struct quantity_quotient_type;
struct quantity_power_type;
struct boolean_type;
struct record_type;
struct record_alias;

using type_expr = std::variant<
    quantity_type,
    quantity_product_type,
    quantity_quotient_type,
    quantity_power_type,
    boolean_type,
    record_type,
    record_alias>;

using expr = std::shared_ptr<type_expr>;

template <typename T, typename... Args>
expr make_type_expr(Args&&... args) {
    return expr(new T(std::forward<Args>(args)...));
}

enum class quantity {
    real,
    length,
    mass,
    time,
    current,
    amount,
    temperature,
    charge,
    frequency,
    voltage,
    resistance,
    capacitance,
    force,
    energy,
    power,
    area,
    volume,
    concentration
};

struct quantity_type {
    quantity type;
};

struct quantity_product_type {
    expr lhs;
    expr rhs;
};

struct quantity_quotient_type {
    expr lhs;
    expr rhs;
};

struct quantity_power_type {
    expr lhs;
    int pow;
};

struct boolean_type {};

struct record_type {
    std::vector<expr> fields;
};

struct record_alias {
    std::string name;
};

} // namespace types
} // namespace al