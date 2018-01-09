#include "cpp-typename-parser.h"
#include <iostream>

struct TestClass{};

void inspect_type( const parser::type& type )
{
	static const char* layer_type2string[] = {
		"PLAIN TYPE"
		, "POINTER"
		, "LVALUE REF"
		, "RVALUE REF"
		, "MEMBER POINTER"
		, "FUNCTION"
		, "ARRAY"
	};
	
	printf("Inspection of Type (from innermost to outermost layer):\n");
	
	for( auto& layer : type ){
		printf(
			" - [%s] %s%s%s%s%s\n"
			, layer_type2string[ (int)layer.layer_type ]
			, layer.is_const ? "const " : ""
			, layer.is_volatile ? "volatile " : ""
			, layer.content.empty() ? "" : "\""
			, layer.content.c_str()
			, layer.content.empty() ? "" : "\""
		);
		for( const auto& arg : layer.arguments )
			printf("   - %s\n" , arg->to_string().c_str() );
	}
	printf("\n");
}

int main( int argc , char** argv )
{
	// TEST FROM A CONCRETE TYPE
	parser::type result1 = parser::type::from_type<unsigned int const (::TestClass::*const)(int,const double&)>();
	inspect_type( result1 );
	
	std::cout << "to_string(): " << result1.to_string("myVar") << std::endl;
	
	printf("\n");
	
	// TEST FROM A TYPE STRING
	parser::type result2 = parser::type("unsigned int const (::TestClass::*const)(int, const double)");
	inspect_type( result2 );
	
	std::cout << "to_string(): " << result2.to_string("myVar") << std::endl;
}