// Copyright (c) 2018 Jakob Riedle (DuffsDevice)
// All rights reserved. Source: github.com/DuffsDevice/cpp-typename-parser

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _CPP_TYPENAME_PARSER_H_
#define _CPP_TYPENAME_PARSER_H_

#include <vector>
#include <cstring>
#include <memory> // For std::shared_ptr and std::unique_ptr

// For detail::demangle
#if defined(__GNUG__) && !defined(__clang__)
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#endif

namespace parser
{
	namespace detail
	{
		static std::string to_string( int value , int base = 10 )
		{
			static const char numbers[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			bool is_negative = value < 0;
			if( is_negative )
				value = -value;
			char memory[34];
			char* result = &( memory[33] = 0 );
			do{
				*--result = numbers[value % base];
				value /= base;
			}
			while( value != 0 );
			if( is_negative )
				*--result = '-';
			return std::string( result );
		}
		
		/**
		 * Function that converts a type into string
		 */
		#if defined(__GNUG__) && !defined(__clang__)
		static inline std::string demangle(const char* name){
			int status = -4; // Some arbitrary value to eliminate the compiler warning
			std::unique_ptr<char, void(*)(void*)> res {
				abi::__cxa_demangle(name, NULL, NULL, &status),
				std::free
			};
			return (status==0) ? res.get() : name ;
		}
		#else
		static inline std::string demangle(const char* name){ return name; } // Do nothing if not g++
		#endif

		template<class T>
		std::string get_typename() {
			return demangle( typeid(T).name() );
		}
		
		struct variadic_comma_type{ template <typename... T1> variadic_comma_type(T1&&...){} };
	}
	
	/**
	 * Use this class to parse (using parser::type("const int (*)[4]") )
	 * or to generate (using myType.to_string()) C++ typenames
	 */
	class type
	{
	private:
		
		enum class layer_type
		{
			type // <node_type>
			, pointer // <node_ptr_or_ref>.1
			, lvalue // <node_ptr_or_ref>.2
			, rvalue // <node_ptr_or_ref>.3
			, member_pointer // <node_mem_ptr>
			, function // <node_array_func>.1
			, array // <node_array_func>.2
		};

		struct layer
		{
			layer_type							layer_type;
			std::string							content;
			bool 								is_const;
			bool 								is_volatile;
			std::vector<std::shared_ptr<type>>	arguments;
			
			bool operator==( const layer& other ) const {
				return
					layer_type == other.layer_type
					&& is_const == other.is_const
					&& is_volatile == other.is_volatile
					&& content == other.content
					&& arguments == other.arguments
				;
			}
			bool operator!=( const layer& other ) const { return *this == other; }
		};
		
		std::vector<layer> layers;
		
	public:
		
		//! Default Ctor
		type() : layers( 1 , { layer_type::type , content : "void" } ) {}
		
		//! Ctor from C++ typename in string form
		type( const char* val ){ if( val ) node_type( val ); }
		type( const std::string& val ) : type( val.c_str() ) {}
		type( const type& ) = default;
		type( type&& ) = default;
		type& operator=( const type& ) = default;
		type& operator=( type&& ) = default;
		
		//! Comparison operator
		bool operator==( const type& other ) const { return layers == other.layers; }
		bool operator!=( const type& other ) const { return layers != other.layers; }
		
		//! Boolean conversion
		explicit operator bool() const { return !layers.empty(); }
		
		//! Iterator Interface
		std::vector<layer>::iterator begin(){ return layers.begin(); }
		std::vector<layer>::const_iterator begin() const { return layers.begin(); }
		std::vector<layer>::const_iterator cbegin(){ return layers.cbegin(); }
		std::vector<layer>::iterator end(){ return layers.end(); }
		std::vector<layer>::const_iterator end() const { return layers.end(); }
		std::vector<layer>::const_iterator cend(){ return layers.cbegin(); }
		std::vector<layer>::reverse_iterator rbegin(){ return layers.rbegin(); }
		std::vector<layer>::const_reverse_iterator rbegin() const { return layers.rbegin(); }
		std::vector<layer>::const_reverse_iterator crbegin(){ return layers.crbegin(); }
		std::vector<layer>::reverse_iterator rend(){ return layers.rend(); }
		std::vector<layer>::const_reverse_iterator rend() const { return layers.rend(); }
		std::vector<layer>::const_reverse_iterator crend(){ return layers.crbegin(); }
		
		//! Convert this structure to a string representation (possibly to declare a variable 'name')
		std::string to_string( std::string name = {} ) const
		{
			std::vector<std::string>	result;
			layer						last_layer;
			size_t						insert_pos = 0;
			
			for( const layer& lr : layers ){
				switch( lr.layer_type ){
					case layer_type::type:
						if( lr.is_const )
							result.emplace( result.begin() + insert_pos++ , "const");
						if( lr.is_volatile )
							result.emplace( result.begin() + insert_pos++ , "volatile");
						result.emplace( result.begin() + insert_pos++ , lr.content );
						break;
					case layer_type::array:
						result.emplace( result.begin() + insert_pos , "]" );
						result.emplace( result.begin() + insert_pos , lr.content );
						result.emplace( result.begin() + insert_pos , "[" );
						break;
					case layer_type::function:{
						result.emplace( result.begin() + insert_pos , ")" );
						bool is_first = true;
						for( auto it = lr.arguments.rbegin() ; it != lr.arguments.rend() ; it++ ){
							if( !is_first )
								result.emplace( result.begin() + insert_pos , "," );
							else
								is_first = false;
							result.emplace( result.begin() + insert_pos , (*it)->to_string() );
						}
						result.emplace( result.begin() + insert_pos , "(" );
						break;
					}
					case layer_type::pointer:
					case layer_type::lvalue:
					case layer_type::rvalue:
					case layer_type::member_pointer:
						static const char* lut[] = { "" , "*" , "&" , "&&" };
						bool need_parens =
							last_layer.layer_type == layer_type::array
							|| last_layer.layer_type == layer_type::function
							|| ( !is_primitive_type(result[insert_pos-1]) && lr.content.front() == ':' )
						;
						if( need_parens )
							result.emplace( result.begin() + insert_pos++ , "(" );
						if( lr.layer_type == layer_type::member_pointer )
							result.emplace( result.begin() + insert_pos++ , lr.content + "::*" );
						else
							result.emplace( result.begin() + insert_pos++ , lut[(int)lr.layer_type] );
						if( lr.is_const )
							result.emplace( result.begin() + insert_pos++ , "const");
						if( lr.is_volatile )
							result.emplace( result.begin() + insert_pos++ , "volatile");
						if( need_parens )
							result.insert( result.begin() + insert_pos , ")" );
						break;
				}
				last_layer = lr;
			}
			
			if( !name.empty() )
				result.emplace( result.begin() + insert_pos , std::move(name) );
			
			std::string output;
			
			for( const auto& str : result ){
				if( need_space( output.back() , str.front() ) )
					output += ' ';
				output += str;
			}
			
			return std::move(output);
		}
		
		template<typename T>
		static type from_type(){
			type result{nullptr};
			from_type_helper<T>::work( result );
			return result;
		}
		
	public: //! MODIFIERS !//
		
		//! Sets the basic data type of this type object
		void set_datatype( type t ){
			if( !layers.empty() && layers.front().layer_type == layer_type::type )
				layers.erase( layers.begin() );
			layers.insert( layers.begin() , t.layers.begin() , t.layers.end() );
		}
		//! Adds a 'const' qualification to this type at the outermost level
		void add_const(){
			if( layers.empty() )
				layers.push_back( { layer_type::type , "void" , true } );
			switch( layers.back().layer_type ){
				case layer_type::type:
				case layer_type::pointer:
				case layer_type::member_pointer:
					layers.back().is_const = true;
					break;
				default:
					break;
			}
		}
		//! Adds a 'volatile' qualification to this type at the outermost level
		void add_volatile(){
			if( layers.empty() )
				layers.push_back( { layer_type::type , "void" , false , true } );
			switch( layers.back().layer_type ){
				case layer_type::type:
				case layer_type::pointer:
				case layer_type::member_pointer:
					layers.back().is_volatile = true;
					break;
				default:
					break;
			}
		}
		//! Adds a array qualification to this type at the outermost level
		void add_array( int extent = -1 ){
			if( layers.empty() )
				layers.push_back( { layer_type::type , "void" } );
			layers.push_back( { layer_type::array , extent > 0 ? detail::to_string(extent) : "" } );
		}
		//! Adds a function qualification to this type at the outermost level
		void add_function( std::vector<std::shared_ptr<type>> parameters = {} ){
			if( layers.empty() )
				layers.push_back( { layer_type::type , "void" } );
			layers.push_back( { layer_type::function , {} , false , false , parameters } );
		}
		//! Removes 'const' qualification of this type at the outermost level
		void remove_const(){
			if( !layers.empty() )
				layers.back().is_const = false;
		}
		//! Removes 'volatile' qualification of this type at the outermost level
		void remove_volatile(){
			if( !layers.empty() )
				layers.back().is_volatile = false;
		}
		//! Removes reference qualification of this type at the outermost level
		void remove_reference(){
			switch( layers.back().layer_type ){
				case layer_type::lvalue:
				case layer_type::rvalue:
					layers.pop_back();
					break;
				default:
					break;
			}
		}
		//! Removes pointer, array or function qualification of this type at the outermost level
		void remove_pointer(){
			switch( layers.back().layer_type ){
				case layer_type::pointer:
				case layer_type::member_pointer:
				case layer_type::function:
				case layer_type::array:
					layers.pop_back();
					break;
				default:
					break;
			}
		}
		//! Resets this type to 'void'
		void clear(){
			*this = type();
		}
	public: //! INFORMATION RETRIEVAL !//
	
		std::string get_datatype() const {
			return !layers.empty() && layers.front().layer_type == layer_type::type ? layers.front().content : std::string();
		}
		bool is_plain() const {
			return layers.size() == 1 && layers.front().layer_type == layer_type::type;
		}
		bool is_lvalue_reference() const {
			return !layers.empty() && layers.back().layer_type == layer_type::lvalue;
		}
		bool is_rvalue_reference() const {
			return !layers.empty() && layers.back().layer_type == layer_type::rvalue;
		}
		bool is_array() const {
			return !layers.empty() && layers.back().layer_type == layer_type::array;
		}
		bool is_pointer() const {
			return !layers.empty() && layers.back().layer_type == layer_type::pointer;
		}
		bool is_member_pointer() const {
			return !layers.empty() && layers.back().layer_type == layer_type::member_pointer;
		}
		bool is_const() const {
			return !layers.empty() && layers.back().is_const;
		}
		bool is_volatile() const {
			return !layers.empty() && layers.back().is_volatile;
		}
		bool is_void() const {
			return layers.size() == 1 && layers.front().content == "void";
		}
		
	private: //! PARSER STUFF !//
		
		static void skip_spaces( const char*& input ){
			while( std::isspace( *input ) )
				input++;
		}
		
		static bool need_space( char lhs , char rhs ){
			if( std::isalpha( lhs ) )
				return std::isalpha( rhs ) || rhs == '*' || rhs == '(' || rhs == ':' ;
			if( lhs == '*' || lhs == ')' || lhs == ':' )
				return std::isalpha( rhs );
			return false;
		}
		
		static bool is_primitive_type( std::string& str ){
			static const char* primitive_types[13] = {
				"char" , "char16_t" , "char32_t" , "wchar_t" , "bool" , "short"
				, "int" , "long" , "signed" , "unsigned" , "float" , "double" , "void"
			};
			for( int i = 0 ; i < 13 ; i++ )
				if( str == primitive_types[i] )
					return true;
			return false;
		}
		
		/**
		 * GRAMMAR:
		 * 
		 * <node_type>			:= <node_basic_type> [ <node_type_qual> ]
		 * <node_basic_type>	:=
		 *  -  { <node_cv_qual> } PRIMITIVE_TYPE { PRIMITIVE_TYPE | <node_cv_qual> }
		 *  -  { <node_cv_qual> } ['::'] <node_name> { '::' <node_name> } { <node_cv_qual> }
		 * <node_name>			:= [a-zA-Z_]+ [ '<' TEMPLATE_PARAMETERS '>' ]
		 * <node_cv_qual>		:= 'const' | 'volatile'
		 * <node_type_qual>		:= <node_ptr_or_ref> [ <node_type_qual> ] | <node_array_func>
		 * <node_ptr_or_ref>	:= '*' { <node_cv_qual> } | '&' | '&&' | <node_mem_ptr>
		 * <node_mem_ptr>		:= ['::'] <node_name> '::' { <node_name> '::' } '*' { <node_cv_qual> }
		 * <node_array_func>	:=
		 *  -  [ <node_array_func> ] '(' PARAMETERS ')' { <node_cv_qual> }
		 *	-  [ <node_array_func> ] '[' CONSTANT ']'
		 *	-  '(' <node_type_qual> ')'
		*/
		bool node_type( const char*& input )
		{
			layers.push_back( { layer_type::type } ); // <node_type>
			skip_spaces( input );
			if( !node_basic_type( input ) ){
				layers.pop_back();
				return false;
			}
			node_type_qual( input );
			return true;
		}
		
		/**
		 * <node_basic_type>	:=
		 *  -  { <node_cv_qual> } PRIMITIVE_TYPE { PRIMITIVE_TYPE | <node_cv_qual> }
		 *  -  { <node_cv_qual> } ['::'] <node_name> { '::' <node_name> } { <node_cv_qual> }
		 */
		bool node_basic_type( const char*& input )
		{
			const char* input_backup;
			std::string content;
			bool		primitive = true;
			
		read_word:
			
			while( node_cv_qual( input ) );
			
			input_backup = input;
			
			if( node_name( input , content ) )
			{
				// If the type is primitive, the type is allowed to consist of more than one word
				bool still_primitive = primitive && is_primitive_type(content);
				
				if( !layers.back().content.empty() && still_primitive )
					layers.back().content += ' ';
				
				if( layers.back().content.empty() || still_primitive ){
					layers.back().content += content;
					content.clear();
					primitive = still_primitive;
					goto read_word;
				}
				input = input_backup;
			}
			else if( // A Class in global scope!
				( !primitive || layers.back().content.empty() )
				&& input[0] == ':'
				&& input[1] == ':'
				&& ( std::isalpha(input[2]) || input[2] == '_' )
			){
				layers.back().content += "::";
				input += 2; // Read the '::'
				node_name( input , layers.back().content );
				goto read_word;
			}
			
			return !layers.back().content.empty();
		}
		
		//! <node_name>			:= [a-zA-Z_]+ [ '<' TEMPLATE_PARAMETERS '>' ]
		bool node_name( const char*& input , std::string& dest )
		{
			if( !std::isalpha(*input) || *input == '_' )
				return false;
			
			do{
				dest += *input++;
			}while( std::isalpha(*input) || *input == '_' );
			
			skip_spaces( input );
			
			if( *input == '<' )
			{
				size_t		orig_len = dest.length();
				const char* input_backup = input;
				int			num_open_brackets = 0;
				
				dest += *input++; // Read in the '<'
				
				while( *input && ( *input != '>' || num_open_brackets > 0 ) ){
					if( *input == '<' )
						num_open_brackets++;
					else if( *input == '>' )
						num_open_brackets--;
					dest += *input++;
				}
				
				// Check Postconditions
				if( !*input ){
					dest.resize( orig_len );
					input = input_backup;
				}
				else{
					dest += input++; // Read the '>'
					skip_spaces( input );
				}
			}
			
			return true;
		}
		
		//! <node_cv_qual>		:= 'const' | 'volatile'
		bool node_cv_qual( const char*& input ){
			if( strncmp( input , "const" , 5 ) == 0 ){
				input += 5;
				layers.back().is_const = true;
			}
			else if( strncmp( input , "volatile" , 8 ) == 0 ){
				input += 8;
				layers.back().is_volatile = true;
			}
			else
				return false;
			skip_spaces( input );
			return true;
		}
		
		//! <node_type_qual>		:= <node_ptr_or_ref> [ <node_type_qual> ] | <node_array_func>
		bool node_type_qual( const char*& input ){
			if( !node_ptr_or_ref( input ) )
				return node_array_func( input );
			while( node_ptr_or_ref( input ) );
			node_array_func( input );
			return true;
		}
		
		//! <node_ptr_or_ref>	:= '*' { <node_cv_qual> } | '&' | '&&' | <node_mem_ptr>
		bool node_ptr_or_ref( const char*& input ){
			if( *input == '*' ){
				layers.push_back( { layer_type::pointer } ); // <node_ptr_or_ref>.1
				input++;
				skip_spaces( input );
				while( node_cv_qual( input ) );
				return true;
			}
			if( *input == '&' ){
				if( input[1] == '&' ){
					layers.push_back( { layer_type::rvalue } ); // <node_ptr_or_ref>.3
					input++;
				}
				else
					layers.push_back( { layer_type::lvalue } ); // <node_ptr_or_ref>.2
				input++;
				skip_spaces( input );
				return true;
			}
			if( node_mem_ptr( input ) )
				return true;
			return false;
		}
		
		//! <node_mem_ptr>		:= ['::'] <node_name> '::' { <node_name> '::' } '*' { <node_cv_qual> }
		bool node_mem_ptr( const char*& input )
		{
			const char* backup = input;
			std::string content;
			
		start:
			if( input[0] == ':' && input[1] == ':' ){
				input+=2;
				content += "::";
				skip_spaces( input );
				goto start;
			}
			else if( node_name( input , content ) )
				goto start;
			
			if( content.empty() || content.back() != ':' || *input != '*' ){ // Backtrace
				input = backup;
				return false;
			}
			
			// Remove the trailing '::'
			content.resize( content.size() - 2 );
			
			input++; // Read in the '*'
			skip_spaces( input );
			
			layers.push_back( { layer_type::member_pointer , content } ); // <node_type>
			
			while( node_cv_qual( input ) );
			
			// skip_spaces( input ); // Unnecessary, since node_ptr_or_ref also invokes skip_spaces( input ) after call to node_mem_ptr
			
			return true;
		}
		
		/**
		 * <node_array_func>	:=
		 *  -  [ <node_array_func> ] '(' PARAMETERS ')' { <node_cv_qual> }
		 *	-  [ <node_array_func> ] '[' CONSTANT ']'
		 *	-  '(' <node_type_qual> ')'
		 */
		bool node_array_func( const char*& input )
		{
			if( !*input )
				return false;
			
			size_t insert_pos = layers.size();
			
			for( int num_tries = 0; true ; num_tries++ )
			{
				const char* input_backup = input;
				for( int cur_try = 0 ; cur_try < num_tries ; cur_try++ )
				{
					if( *input == '[' ) // Must be array
					{
						input++;
						skip_spaces( input );
						int open_parens = 0;
						std::string content;
						while( *input && ( *input != ']' || open_parens > 0 ) ){
							if( *input == '[' )
								open_parens++;
							else if( *input == ']' )
								open_parens--;
							content += *input++;
						}
						if( !*input )
							return false;
						layers.insert( layers.begin() + insert_pos , { layer_type::array ,content } ); // <node_array_func>.2
						input++; // Read the ']'
						skip_spaces( input );
					}
					else if( *input == '(' )
					{
						input++;
						skip_spaces( input );
						const char* input_backup_2 = input;
						if( cur_try == 0 && node_type_qual( input ) && *input == ')' ){
							input++;
							skip_spaces( input );
						}
						else{ // Must be PARAMETERS
							input = input_backup_2;
							auto res = layers.insert( layers.begin() + insert_pos , { layer_type::function } ); // <node_array_func>.2
							while(true){
								type param{nullptr};
								const char* input_backup_3 = input;
								param.node_type(input); // Read in one parameter
								if( param.layers.empty() ){
									input = input_backup_3;
									break;
								}
								res->arguments.push_back( std::make_shared<type>( std::move(param) ) );
								if( *input != ',' )
									break;
								input++; // Read the ','
								skip_spaces(input);
							}
							if( *input != ')' )
								goto breakout;
							
							input++; // Read the ')'
							skip_spaces( input );
							while( node_cv_qual( input ) );
						}
					}
					else
						goto breakout;
					
					continue;
					
				breakout:
					
					// Backtrack and restore
					input = input_backup;
					layers.resize( insert_pos );
					return false;
				}
				if( !*input || *input == ')' || *input == ',' )
					return num_tries > 0;
				
				// Backtrack and restore
				input = input_backup;
				layers.resize( insert_pos );
			}
			
			return true; // Dead code
		}
		
	private:
		
		template<typename T, bool = std::is_reference<T>::value || std::is_array<T>::value || std::is_pointer<T>::value>
		struct from_type_helper{ static void work( type& dest ){
			dest.layers.push_back( { layer_type::type , detail::get_typename<T>() } );
		} };
		template<typename T>
		struct from_type_helper<const T,false>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.back().is_const = true;
		} };
		template<typename T>
		struct from_type_helper<volatile T,false>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.back().is_volatile = true;
		} };
		template<typename T, bool B>
		struct from_type_helper<T*,B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::pointer } );
		} };
		template<typename T, bool B>
		struct from_type_helper<T&,B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::lvalue } );
		} };
		template<typename T, bool B>
		struct from_type_helper<T&&,B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::rvalue } );
		} };
		template<typename T, bool B>
		struct from_type_helper<T[],B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::array } );
		} };
		template<typename T, size_t N, bool B>
		struct from_type_helper<T[N],B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::array , detail::to_string(N) } );
		} };
		template<typename T, class C, bool B>
		struct from_type_helper<T (C::*),B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::member_pointer , detail::get_typename<C>() } );
		} };
		template<typename T, typename... A, bool B>
		struct from_type_helper<T(A...),B>{ static void work( type& dest ){
			from_type_helper<T>::work( dest );
			dest.layers.push_back( { layer_type::function } );
			detail::variadic_comma_type dummy{ (dest.layers.back().arguments.emplace_back( std::make_shared<type>( type::from_type<A>() ) ), 0)... };
		} };
	};
	
} // namespace parser

#endif