//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2017, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "boost/format.hpp"

#include "IECoreScene/MeshAlgo.h"
#include "IECore/DespatchTypedData.h"

using namespace Imath;
using namespace IECore;
using namespace IECoreScene;

namespace
{

class Segmenter
{
	public:
		Segmenter( const MeshPrimitive &mesh, Data *data, const IntVectorData *indices ) : m_mesh( mesh ), m_data( data ), m_indices( indices )
		{
		}

		typedef std::vector<MeshPrimitivePtr> ReturnType;

		template<typename T>
		ReturnType operator()( T *array )
		{
			T *segments = IECore::runTimeCast<T>( m_data );

			if ( !segments )
			{
				throw IECore::InvalidArgumentException(
					(
						boost::format( "Segment keys type '%s' doesn't match primitive variable type '%s'" ) %
							m_data->typeName() %
							array->typeName()
					).str()
				);
			}

			const auto &segmentsReadable = segments->readable();

			ReturnType results;
			results.reserve( segmentsReadable.size() );
			const auto &readable = array->readable();

			BoolVectorDataPtr deletionArray = new BoolVectorData();
			auto &writable = deletionArray->writable();
			writable.resize( readable.size() );

			for( auto segment : segmentsReadable )
			{
				if( m_indices )
				{
					auto &readableIndices = m_indices->readable();

					for( size_t i = 0; i < readable.size(); ++i )
					{
						size_t index = readableIndices[i];
						writable[i] = segment != readable[index];
					}
				}
				else
				{
					for( size_t i = 0; i < readable.size(); ++i )
					{
						writable[i] =  segment != readable[i];
					}
				}

				IECoreScene::PrimitiveVariable delPrimVar( IECoreScene::PrimitiveVariable::Uniform, deletionArray );
				results.push_back( MeshAlgo::deleteFaces( &m_mesh, delPrimVar, false ) );
			}

			return results;
		}

	private:
		const MeshPrimitive &m_mesh;
		Data *m_data;
		const IntVectorData *m_indices;

};

} // namespace



std::vector<MeshPrimitivePtr> IECoreScene::MeshAlgo::segment( const MeshPrimitive *mesh, const IECore::Data *data, const PrimitiveVariable &primitiveVariable )
{
	Segmenter segmenter( *mesh, const_cast<IECore::Data*> (data), primitiveVariable.indices.get() );

	return despatchTypedData<Segmenter, IECore::TypeTraits::HasVectorValueType>(  primitiveVariable.data.get(), segmenter );
}