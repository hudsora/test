#include <stdio.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <future>
#include <chrono>
#include <random>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>

		// compile with g++-4.8 -O3 -std=c++0x testsort.cpp -o tso
			// note, will run with gcc 4.7, but reported times will be off by a factor of 1000

	#define	TEST	1

	using std::cout ;
	using std::cerr ;

	using std::chrono::high_resolution_clock ;

	const static int	kMaxPrintLength = 160 ;

	/// data type and container

typedef	 double	datatype ;
typedef	 std::vector<datatype>	containertype ;

typedef	std::chrono::duration<double, std::milli>	timespan ;

	/// read data

containertype	read_numbers(FILE * fsrc)
{
	char strline[256], * p, c ;
	containertype	loadvec ;

	while ( fgets( strline, sizeof( strline), fsrc) )
	{
		p= strline ;

			// strip out leading whitespace
		while ( p && ( c = * p  ) && isspace( c) ) ++ p ;
			// test for valid number
		if ( p && ( isdigit( c) || ( '+' == c) || ( '-' == c ) || ( '.' == c )))
			{ loadvec.push_back( atof( p)) ; }

	}

	return loadvec ;
}


	/// 

		// helper template for printing
	template <typename Titer> void	print_range(Titer const & afirst, Titer const & alast )
	{
		cout << '[' ;
		std::for_each( afirst, alast, []( datatype aval ) { cout << ' ' << aval << ',' ; } ) ;
		cout << "]\n" ;
	}

		// abstract base classes for organizing sort operations
class	sorters
{
	public:
		sorters() { getsorters_().push_back( this) ; }
		virtual ~ sorters() {}

		virtual	const char *	name(void) const = 0 ;
		virtual void			sort(void) = 0 ;
		virtual void			doprint(void) const = 0 ;

		static std::vector<sorters *> &	getsorters_(void) ;

		timespan	elapsed_ ;
} ;

class	container_sorter : public sorters
{
	public:
		container_sorter(const containertype & alist) : sort_( alist) { }
		void	doprint(void) const { print_range(sort_.begin(), sort_.end()) ; }

	protected:
		containertype	sort_ ;
} ;

class	raw_sorter : public sorters
{
	public:
		raw_sorter(const containertype & alist) : sort_( new datatype[ alist.size() ] ), size_( alist.size())
			{ std::copy( alist.begin(), alist.end(), sort_.get() ) ; }
		void	doprint(void) const { print_range( & (sort_[0]), & (sort_[size_]) ) ; }

	protected:
		std::unique_ptr< datatype [] >	sort_ ;
		size_t	size_ ;
} ;


		// protected static list (pattern to avoid race conditions on initialization)
	std::vector<sorters *> &	sorters::getsorters_(void)
	{
		static std::vector<sorters *>	slist ;
		return slist ;
	}

// boilerplate for specific sorts

#define		SORTER(aclassname, aparent, astring)	struct aclassname : public aparent \
			{ aclassname( const containertype & alist ) : aparent( alist) { } \
				const char * name(void) const { return astring ; }  void sort(void) ; }

// define specific sorts

		SORTER( bubble_sort, container_sorter, "bubble sort" ) ;
		SORTER( raw_bubble_sort, raw_sorter, "raw bubble sort" ) ;
		SORTER( insertion_sort, container_sorter, "insertion sort" ) ;
		SORTER( raw_insertion_sort, raw_sorter, "raw insertion sort" ) ;
		SORTER( selection_sort, container_sorter, "selection sort" ) ;
		SORTER( raw_selection_sort, raw_sorter, "raw selection sort" ) ;
		SORTER( shell_sort, container_sorter, "shell sort" ) ;
		SORTER( raw_shell_sort, raw_sorter, "raw shell sort" ) ;
		SORTER( raw_qsort, raw_sorter, "system qsort" ) ;
		SORTER( stl_sort, container_sorter, "stl sort" ) ;

	// do merge sort special because of recursive helper funciton
struct merge_sort : public container_sorter
{
	merge_sort( const containertype & alist ) : container_sorter( alist) { } 
	const char * name(void) const { return "merge sort" ; }

	void sort(void) ;

	private:
		void sort_range( long ibegin, long iend ) ;
} ;

struct raw_merge_sort : public raw_sorter
{
	raw_merge_sort( const containertype & alist ) : raw_sorter( alist ) { }
	const char * name(void) const { return "raw merge sort" ; }

	void sort(void) ;

	private:
		void sort_range( datatype * pfirst, datatype * plast ) ;
} ;

struct buffered_merge_sort : public raw_sorter
{
	buffered_merge_sort( const containertype & alist )
			: raw_sorter( alist ),
				buffer_( new datatype[ ( alist.size() / 2 ) + 1 ] ),
				bufsize_( ( alist.size() / 2 ) + 1 )
	{ }

	const char * name(void) const { return "buffered merge sort" ; }

	void sort(void) ;

	private:
		void sort_range( datatype * pfirst, datatype * plast ) ;

		std::unique_ptr< datatype [] >  buffer_ ;
		size_t	bufsize_ ;
} ;

	////
	//
	// implementation for sort functions

void	bubble_sort::sort( void)
{
	int ctswaps ;
	auto plimit= sort_.end() ;

	do
	{
		auto p1= sort_.begin() ;
		auto p2= p1 + 1;

		ctswaps= 0 ;
		while ( p2 < plimit )
		{
				// compare adjacent values and swap if out of order
			if ( * p2 < * p1 ) { std::iter_swap( p1, p2) ;  ctswaps ++ ; }
			++ p1, ++ p2 ;
		}
	}
			// keep starting over if any values have be re-arranged
		while ( ctswaps ) ;
}

void	raw_bubble_sort::sort( void)
{
	int ctswaps ;
	datatype * p1, * p2, * plimit ;

	plimit= & ( sort_[size_] ) ;

	do
	{
		p1= sort_.get(), p2= p1 +1, ctswaps= 0 ;
		while ( p2 < plimit )
		{
			if ( * p2 < * p1 ) { std::iter_swap( p1, p2) ;  ctswaps ++ ; }
			++ p2, ++ p1 ;
		}
	}
		while ( ctswaps ) ;
}

void	insertion_sort::sort( void)
{
	auto pstep= sort_.begin() ;
	auto pbegin= pstep ;
	auto plimit= sort_.end() ;
	datatype dvalue ;

	++ pstep ;
	while ( pstep < plimit )
	{
		auto psear= pstep -1 ;
		dvalue = * pstep ;
			// look for the spot that * pstep should be in the already sorted list 
		while (( psear >= pbegin ) && ( * psear > dvalue )) { -- psear ; }
		
		++ psear ;
			// rotate larger elements around behind * pstep
		if ( psear < pstep ) { std::rotate( psear, pstep, pstep +1) ; }
		++ pstep ;
	}
}

void	raw_insertion_sort::sort( void)
{
	datatype * pbegin, * pstep, * psear, * plimit, dvalue ;
	size_t recsize = sizeof( datatype) ;

	pbegin= pstep= sort_.get() ;  pstep ++ ;
	plimit= & ( sort_[size_] ) ;

	while ( pstep < plimit )
	{
		psear= pstep -1, dvalue= * pstep ;
		while (( psear >= pbegin ) && ( * psear > dvalue )) { psear -- ; } 

		psear ++ ;
		if ( psear < pstep ) {
			std::memmove( psear +1, psear, recsize * ( pstep - psear )) ;
			* psear= dvalue ;
		}
		pstep ++ ;
	}
}

void	selection_sort::sort( void)
{
	auto pstep= sort_.begin() ;
	auto plimit= sort_.end() ;

	while ( pstep < plimit )
	{
			// look for a lower value in the rest of the list than what's at * pstep
		auto plook= pstep, pbest= pstep ;
		++ plook ;
		while ( plook < plimit )
		{
			if ( * plook < * pbest ) pbest= plook ;
			++ plook ;
		}

		if ( pbest != pstep ) { std::iter_swap( pstep, pbest) ; }
		++ pstep ; 
	}
}

void	raw_selection_sort::sort( void)
{
	datatype * pstep, * plook, * pbest, * plimit, dvalue ;

	pstep= sort_.get() ;
	plimit= & ( sort_[size_] ) ;

	while ( pstep < plimit )
	{
		plook= pbest= pstep ;  ++ plook ;
		while ( plook < plimit )
		{
			if ( * plook < * pbest ) pbest= plook ;
			++ plook ;
		}

		if ( pbest != pstep ) { dvalue= * pstep, * pstep= * pbest, * pbest= dvalue ; }
		++ pstep ; 
	}
}

void	merge_sort::sort_range( long istart, long iend )
{
	if ( ( iend - istart ) > 1 )
	{
		auto piter= sort_.begin() ;
		long imiddle= ( istart + iend ) / 2 ;

			// divide and sort left and right half
		sort_range( istart, imiddle ) ;
		sort_range( imiddle, iend) ;

			// merge
		std::inplace_merge( piter + istart, piter + imiddle, piter+ iend) ;
	}
}

void	merge_sort::sort( void)
{
		// call recursive helper function
	sort_range( 0, sort_.size()) ;
}

void	raw_merge_sort::sort_range( datatype * pfirst, datatype * plast )
{
	size_t tmpspan= plast - pfirst ;

	if ( tmpspan > 1 )
	{
		datatype * pmiddle = pfirst + ( tmpspan / 2 ) ;

		sort_range( pfirst, pmiddle) ;
		sort_range( pmiddle, plast) ;

		datatype dvalue ;
		size_t recsize = sizeof( datatype) ;

			// in-place merge
		while ( ( pfirst < pmiddle ) && ( pmiddle < plast ))
		{
			if ( * pfirst < * pmiddle ) { ++ pfirst ;  continue ; }

				// shuffle remainder of left around and insert one from the right
			dvalue= * pmiddle ;
			std::memmove( pfirst +1, pfirst, recsize * ( pmiddle - pfirst )) ;

			*( pfirst ++)= dvalue ;  ++ pmiddle ;
		}
	}
}

void	raw_merge_sort::sort( void)
{
	sort_range( & (sort_[0]), & (sort_[size_])) ;
}

void	buffered_merge_sort::sort_range( datatype * pfirst, datatype * plast )
{
	size_t tmpspan= plast - pfirst ;

	if ( tmpspan < 2 ) return ;

	datatype * pmiddle = pfirst + ( tmpspan / 2 ) ;

	sort_range( pfirst, pmiddle) ;
	sort_range( pmiddle, plast) ;

	datatype * pbufstep, * pbuflimit ;
	tmpspan= pmiddle - pfirst ;
	assert( bufsize_ >= tmpspan ) ;

		// copy first half to the buffer space
	std::memcpy( & (buffer_[0] ), pfirst, tmpspan * sizeof( datatype)) ;
	pbufstep= & ( buffer_[0] ), pbuflimit= pbufstep + tmpspan ;

		// merge the second half and the buffered half together
	while ( ( pmiddle < plast ) && ( pbufstep < pbuflimit ))
	{
		if ( * pbufstep < * pmiddle ) { *( pfirst ++)= *( pbufstep ++) ; }
			else { *( pfirst ++)= * ( pmiddle ++ ) ; }
	}
		// copy any remainders of the buffered list over
	while ( pbufstep < pbuflimit ) { *( pfirst ++)= *( pbufstep ++) ; }
}

void	buffered_merge_sort::sort( void)
{
	sort_range( & (sort_[0]), & (sort_[size_])) ;
}

void	shell_sort::sort( void)
{
	auto plimit= sort_.end() ;
	unsigned long hgap = sort_.size() / 2 ;

		// go through list in steps and do insertion sort on each sub-piece
	while ( hgap )
	{
		auto pstep= sort_.begin() ;
		auto pnear= plimit - hgap ;

		while ( pstep < pnear )
		{
			auto pbest= pstep ;
			auto pskip= pstep + hgap ;

				// look for lower value in interlieved list and swap if found
			while ( pskip < plimit ) {
				if ( * pskip < * pbest ) { pbest= pskip ; }
				pskip += hgap ;
			}
			if ( pbest != pstep ) { std::iter_swap( pstep, pbest) ; }
			++ pstep ;
		}

			// shrink gap each time by magic factor
		if ( hgap > 3 ) { hgap /= 2.2 ; } else { hgap= ( hgap > 1 ) ? 1 : 0 ; }
	}
}

void	raw_shell_sort::sort( void)
{
	datatype * pstep, * pbest, * pskip, * plimit, dvalue ;
	unsigned long hgap= size_ / 2 ;

	plimit= & (sort_[size_]) ;

	while ( hgap )
	{
		pstep= sort_.get() ; 
		while ( pstep < plimit )
		{
			pbest= pstep, pskip= pstep + hgap ;
			while ( pskip < plimit ) {
				if ( * pskip < * pbest ) { pbest= pskip ; }
				pskip += hgap ;
			}
			if ( pbest != pstep ) { dvalue= * pstep, * pstep= * pbest, * pbest= dvalue ; }
			++ pstep ;
		}

		if ( hgap > 3 ) { hgap /= 2.2 ; } else { hgap= ( hgap > 1 ) ? 1 : 0 ; }
	}
}

	static int	compare_func( const datatype * pa, const datatype * pb )
					{ datatype delt= * pa - * pb ;  return ( delt > 0 ) ? 1 : (( delt < 0 ) ? -1 : 0 ) ; }

		// default pointer types to qsort void *, not datatype * 
	typedef int ( * compare_func_t ) ( const void *, const void * ) ;

			// system qsort
void	raw_qsort::sort( void)
{
	qsort( sort_.get(), size_, sizeof( datatype), ( compare_func_t ) compare_func) ;
}

			// standard template library sort
void	stl_sort::sort( void)
{
	std::sort( sort_.begin(), sort_.end()) ;
}

	////
	// the main event - instanciate each sorter, run sorts and display times

void	simple_test(const containertype & alist)
{
	bubble_sort	sbu( alist) ;
	raw_bubble_sort	rbu( alist) ;
	insertion_sort sis( alist) ;
	raw_insertion_sort ris( alist) ;
	selection_sort ssl( alist) ;
	raw_selection_sort rsl( alist) ;
	merge_sort sme( alist) ;
	raw_merge_sort rme( alist) ;
	buffered_merge_sort bme( alist) ;
	shell_sort ssh( alist) ;
	raw_shell_sort rsh( alist) ;
	raw_qsort rqs( alist) ;
	stl_sort sst( alist) ;

	for ( auto & sorter : sorters::getsorters_() )
	{
		cout << "try sort: " << sorter-> name() ;
		auto tstart= high_resolution_clock::now() ;
		sorter-> sort() ;
		sorter-> elapsed_ = high_resolution_clock::now() - tstart ;

		cout << " time: " << std::setprecision( 9) << 0.1 * std::floor( 10. * sorter-> elapsed_.count()) << " ms\n" ;
		if ( alist.size() < kMaxPrintLength ) { sorter-> doprint() ; }
	}

	cout << "\n" ;
}

	// parse command line args, load data and run test
int	main(int N, char ** S)
{
	char * prgname= *( S ++ ) ;
	containertype	data ;

	N -- ;

	if ( ! N )
	{
		cerr << "Usage: " << prgname << " <filename>\n"
			<< "\tOR " << prgname << " -\tread list from stdin\n" ;
		exit( 1) ;
	}

#if TEST
	if ( ! strcmp( * S, "-t" ) && ( N > 1 ))
	{
		int icount= atoi( S[1]) ;
		std::random_device	rd ;
		std::mt19937 gen( rd()) ;
		std::uniform_real_distribution<> spread( -100., 100.) ;

		if ( icount)
		{
			data.reserve( icount) ;
			while ( icount -- ) { data.push_back( spread( gen)) ; }

			simple_test( data) ;
			exit( 0) ;
		}
			else { cerr << "invalid test count\n" ;  exit( 11) ; }
	}
#endif

		// determine numbers source and read in list
	if ( strcmp( * S, "-" ) ) {
		FILE * fin= fopen( * S, "r") ;
		if ( fin ) { data= read_numbers( fin) ;   fclose( fin) ; }
			else { cerr << "no numbers read: " << std::strerror( errno) << "\n" ;  exit( 2) ; }
	}
		else { data= read_numbers( stdin) ; }

	if ( ! data.size() ) { cerr << "no numbers read.\n" ;  exit( 3) ; }

	simple_test( data) ;
}

