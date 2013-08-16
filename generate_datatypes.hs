#!/bin/env runhaskell
{-# LANGUAGE DeriveDataTypeable #-}

import Language.Haskell.Exts
import Data.Typeable
import Data.Data
import Data.Map()
import Data.Char
import Data.List

import System.Environment
import System.FilePath
import System.Directory

import Text.StringTemplate
import Text.StringTemplate.GenericStandard

data TDataTypeConstr = TDataTypeConstr{ tdconstrName :: String, flds :: [(String, String)]  } deriving (Show, Eq, Data, Typeable)
data TDataType = TDataType{ dtname :: String, constrs :: [TDataTypeConstr] } deriving (Show, Eq, Data, Typeable)

-- FIXME: add support for generic types (Option X, [X], Either X Y, etc.)
tname dt  = case dt of
                 ParseOk (DataDecl _ DataType _ (Ident name) _ constrs _) -> 
                   TDataType name $ flip map constrs $ \constr ->
                     case constr of
                          QualConDecl _ _ _ (ConDecl (Ident name) fields) -> TDataTypeConstr name $ flip map fields $ \field ->
                            case field of 
                                 UnBangedTy (TyCon (UnQual (Ident name))) -> ("", name)
                                 _ -> error "only unqualified unbanged fields supported, sorry =("
                          QualConDecl _ _ _ (RecDecl (Ident name) fields) -> TDataTypeConstr name $ concat $ flip map fields $ \field ->
                            case field of 
                                 (fieldnames, UnBangedTy (TyCon (UnQual (Ident name)))) -> flip map fieldnames $
                                    \fieldname ->
                                      case fieldname of
                                           (Ident i) -> (i, name)
                                           _ -> error "only identifiers in record fields supported, sorry =("
                                 _ -> error "only unqualified unbanged fields supported, sorry =("
                          _ -> error "only parameterless datatypes supported, sorry =("
                 _ -> error "only datatypes supported, sorry =("

data Meta = Meta { tp :: TDataType, child :: TDataTypeConstr, guard :: String, fname :: String, dir :: String, source_expr :: String, source_file :: String } deriving (Show, Eq, Data, Typeable)

zipWithIndex :: [a] -> [(a, Int)]
zipWithIndex lst = zip lst [0..]

zipWithExt :: FilePath -> (String, String) -> FilePath
zipWithExt dir x = combine dir $ fst x ++ snd x


tmplt <+= (k, v) = setAttribute k v tmplt

-- C++ shit, subject to abstraction

code_extension = ".h"
additional_extension = ".cpp"

data CppTypeDescription = CppTypeDescription { storeName :: String, protobufName :: String, parameterName :: String, builtin :: Bool, importString :: String } deriving (Show, Eq, Data, Typeable)

adjustType "String" = CppTypeDescription "std::string" "string" "const std::string&" True "#include <string>"
adjustType "Int" = CppTypeDescription "int" "sint32" "int" True ""
adjustType "UInt" = CppTypeDescription "unsigned int" "uint32" "unsigned int" True ""
adjustType "LongLong" = CppTypeDescription "long long int" "sint64" "long long int" True ""
adjustType "ULongLong" = CppTypeDescription "unsigned long long int" "uint64" "unsigned long long int" True ""
adjustType "Bool" = CppTypeDescription "bool" "bool" "bool" True ""
adjustType "Float" = CppTypeDescription "double" "double" "double" True ""
adjustType dt = CppTypeDescription (dt ++ "::Ptr") dt (dt ++ "::Ptr") False ""


data CppField = CppField{ fieldName :: String, getterName :: String, typeName :: String, description :: CppTypeDescription } deriving (Show, Eq, Data, Typeable)
data CppDerivedDescriptor = CppDerivedDescriptor{ constrName :: String, fields :: [CppField]} deriving (Show, Eq, Data, Typeable)

makeGetterName (h:t) = "get" ++ [toUpper h] ++ t

adjustDesc (TDataTypeConstr x y) = CppDerivedDescriptor x $ map (\(a,b) -> CppField a (makeGetterName a) b $ adjustType b) y

-- end of C++ shit

data TwoExtensionFileName = TEFN{ thrine :: String, extension :: String, secondExtension :: String } deriving (Show, Eq)

mkTEFN :: FilePath -> TwoExtensionFileName
mkTEFN fp = let (n,se) = splitExtension fp
                (t, e) = splitExtension n 
            in TEFN t e se

zipTEFNWithExt :: FilePath -> TwoExtensionFileName -> FilePath
zipTEFNWithExt dir tefn = combine dir $ thrine tefn ++ extension tefn ++ secondExtension tefn

ext2guard :: String -> String
ext2guard []                   = []
ext2guard (x : tail) | x == '.'  = '_'       : ext2guard tail
                     | otherwise = toUpper x : ext2guard tail


main = do
  args <- getArgs
  dirs <- mapM makeRelativeToCurrentDirectory args

  flip mapM (zip dirs args) $ \ (dir, arg) -> do
    dirfiles <- getDirectoryContents dir

    let filterext ext = filter (\ it -> snd it == ext) $ map splitExtension dirfiles
    let (dt_file : _) = filterext ".datatype"
    let templates = filter (\ it -> secondExtension it == ".hst") $ map mkTEFN dirfiles

    let source_file = zipWithExt dir dt_file

    dt_string <- readFile source_file

    let base_files = filter (\ it -> thrine it == "base") templates
    template_base_contents <- mapM (readFile . zipTEFNWithExt dir) base_files
    let template_bases :: [(StringTemplate String, TwoExtensionFileName)]; template_bases = (map newSTMP template_base_contents) `zip` base_files
    let derived_files = filter (\ it -> thrine it == "derived") templates
    template_derived_contents <- mapM (readFile . zipTEFNWithExt dir) derived_files
    let template_deriveds :: [(StringTemplate String, TwoExtensionFileName)]; template_deriveds = (map newSTMP template_derived_contents) `zip` derived_files

    let datatype = tname $ parseDecl dt_string

    flip mapM (template_bases) $ \ (template_base, fname) -> do
      let outExt = extension fname
      let outname = zipWithExt dir (dtname datatype, outExt)
      let include_guard = (map toUpper $ dtname datatype) ++ ext2guard outExt
      let filled_base :: StringTemplate String
          filled_base = template_base <+= ("dir", dir) 
                                      <+= ("guard", include_guard) 
                                      <+= ("fname", outname)
                                      <+= ("source_expr", dt_string)
                                      <+= ("source_file", source_file)
                                      <+= ("type", datatype)
                                      <+= ("constrs", map adjustDesc $ constrs datatype)
                                      <+= ("template", combine dir $ thrine fname <.> extension fname <.> secondExtension fname)
      writeFile outname $ render filled_base
	
    flip mapM (template_deriveds) $ \ (template_derived, fname) -> do
      flip mapM (zipWithIndex $ constrs datatype) $ \ (constr, ix) -> do
      	let outExt = extension fname
        let outname = zipWithExt dir (tdconstrName constr, extension fname)
        let include_guard = (map toUpper $ tdconstrName constr) ++ ext2guard outExt
        let constructor = adjustDesc constr
        let prelude = unlines $ nub $ filter (\it -> length it > 0) $ flip map (fields constructor) $ \ it -> (importString $ description it) 
        let filled_derived = template_derived <+= ("dir", dir) 
                                              <+= ("prelude", prelude)
                                              <+= ("guard", include_guard) 
                                              <+= ("fname", outname)
                                              <+= ("source_expr", dt_string)
                                              <+= ("source_file", source_file)
                                              <+= ("type", datatype)
                                              <+= ("constr", constructor)
                                              <+= ("index", ix+1)
                                              <+= ("index0", ix)
                                              <+= ("template",combine dir $ thrine fname <.> extension fname <.> secondExtension fname)
        writeFile outname $ render filled_derived

    putStrLn "done!"
