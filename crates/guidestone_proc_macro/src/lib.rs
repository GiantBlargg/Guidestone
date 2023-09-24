use proc_macro::TokenStream;
use quote::{quote, quote_spanned};
use syn::{parse_macro_input, parse_quote, spanned::Spanned, Data, DeriveInput, GenericParam};

#[proc_macro_derive(FromRead)]
pub fn from_read_derive(input: TokenStream) -> TokenStream {
	let ast = parse_macro_input!(input as DeriveInput);

	let name = &ast.ident;

	let generics = {
		let mut generics = ast.generics;

		for param in &mut generics.params {
			if let GenericParam::Type(ref mut type_param) = *param {
				type_param.bounds.push(parse_quote!(FromRead))
			}
		}
		generics
	};
	let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();

	let load_each = match ast.data {
		Data::Struct(data) => match data.fields {
			syn::Fields::Named(fields) => {
				let recurse = fields.named.into_iter().map(|f| {
					let name = &f.ident;
					quote_spanned! {f.span()=>
						#name:FromRead::from_read(r)?
					}
				});
				quote! {Ok(Self{#(#recurse),*})}
			}
			syn::Fields::Unnamed(fields) => {
				let recurse = fields.unnamed.into_iter().map(|f| {
					quote_spanned! {f.span()=>
						FromRead::from_read(r)?
					}
				});
				quote! {Ok(Self(#(#recurse),*))}
			}
			syn::Fields::Unit => unimplemented!(),
		},
		Data::Enum(_) | Data::Union(_) => unimplemented!(),
	};

	let gen = quote! {
		impl #impl_generics FromRead for #name #ty_generics #where_clause {
			fn from_read<R: ::std::io::Read>(r: &mut R) -> ::std::io::Result<Self> {
				#load_each
			}
		}
	};

	gen.into()
}
