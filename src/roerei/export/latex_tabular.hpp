#pragma once

#include <ostream>

namespace roerei {
	class latex_tabular
	{
	private:
		std::ostream& os;
		bool ended;

	public:
		template<typename COL>
		latex_tabular(std::ostream& _os, std::initializer_list<COL> const& coltypes)
		: os(_os)
		, ended(false)
		{
			os << "\\begin{tabular}{";
			for(auto const& coltype : coltypes) {
				os << coltype;
			}
			os << "}\n";
		}

		template<typename COL>
		void write_header(std::initializer_list<COL> const& cols)
		{
			write_row(cols);
		}
		
		template<typename COL>
		void write_row(std::initializer_list<COL> const& cols)
		{
			bool first = true;
			for(auto const& col : cols) {
				if(first) {
					first = false;
				} else {
					os << " & ";
				}

				os << col;
			}

			os << "\\\\\n";
		}

		void write_end()
		{
			if(!ended) {
				os << "}\n";
				os.flush();
				ended = true;
			}
		}

		~latex_tabular()
		{
			write_end();
		}
	};
}
