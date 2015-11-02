#!/bin/env runhaskell
{-# LANGUAGE DeriveDataTypeable #-}

import Prelude hiding (unlines)
import qualified Prelude (unlines)

import Language.Haskell.Exts hiding (CPlusPlus)
import Data.Typeable
import Data.Data
import Data.Map(fromList)
import Data.Char
import Data.List hiding (unlines)

import System.Environment
import System.FilePath
import System.Directory

import Text.StringTemplate
import Text.StringTemplate.GenericStandard
import Text.StringTemplate.Classes

unlines [] = ""
unlines ("":[]) = ""
unlines (h:[]) = h
unlines ("":t) = unlines t
unlines (h:t) | last h == '\n' = h ++ unlines t
unlines (h:t) = h ++ "\n" ++ unlines t

data TTypeRef = Single String | TTyApp TTypeRef TTypeRef deriving (Show, Eq, Data, Typeable)

data TDataTypeConstr = TDataTypeConstr{ tdconstrName :: String, flds :: [(String, TTypeRef)]  } deriving (Show, Eq, Data, Typeable)
data TDataType = TDataType{ dtname :: String, constrs :: [TDataTypeConstr] } deriving (Show, Eq, Data, Typeable)

ttype' (TyCon (UnQual (Ident name))) = Single name
ttype' (TyParen x) = ttype' x
ttype' (TyApp x y) = TTyApp (ttype' x) (ttype' y)
ttype' (TyList y)  = TTyApp (Single "[]") (ttype' y)
ttype' _           = error "only unqualified unbanged fields supported, sorry =("

-- FIXME: add support for generic types (Option X, [X], Either X Y, etc.)
tname dt  = case dt of
                 ParseOk (DataDecl _ DataType _ (Ident name) _ constrs _) ->
                   TDataType name $ flip map constrs $ \constr ->
                     case constr of
                          QualConDecl _ _ _ (ConDecl (Ident name) fields) -> TDataTypeConstr name $ flip map fields $ \field -> ("", ttype' field)
                          QualConDecl _ _ _ (RecDecl (Ident name) fields) -> TDataTypeConstr name $ concat $ flip map fields $ \field ->
                            case field of
                                 (fieldnames, rttype) -> flip map fieldnames $
                                    \fieldname ->
                                      case fieldname of
                                           (Ident i) -> (i, ttype' rttype)
                                           _ -> error "only identifiers in record fields supported, sorry =("
                          _ -> error "only parameterless datatypes supported, sorry =("
                 smth -> error ("only datatypes supported, sorry =( : " ++ show smth)

zipWithIndex :: [a] -> [(a, Int)]
zipWithIndex lst = zip lst [0..]

zipWithExt :: FilePath -> (String, String) -> FilePath
zipWithExt dir x = combine dir $ fst x ++ snd x


-- C++ shit, subject to abstraction

code_extension = ".h"
additional_extension = ".cpp"

data CppTypeDescription = CppTypeDescription {
    cppStoreName :: String,
    cppParameterName :: String,
    cppImportStrings :: [String]
} deriving (Show, Eq, Data, Typeable)

instance ToSElem CppTypeDescription where
  toSElem (CppTypeDescription store param imports) = SM $ Data.Map.fromList [
      ("store", toSElem store),
      ("parameter", toSElem param),
      ("imports", toSElem imports)
    ]

cref typename = "const " ++ typename ++ "&"

adjustTypeCpp (Single "String")                     = CppTypeDescription "std::string"            "const std::string&"      ["#include <string>"]
adjustTypeCpp (Single "Int")                        = CppTypeDescription "int"                    "int"                     []
adjustTypeCpp (Single "UInt")                       = CppTypeDescription "unsigned int"           "unsigned int"            []
adjustTypeCpp (Single "Size")                       = CppTypeDescription "size_t"                 "size_t"                  ["#include <cstddef>"]
adjustTypeCpp (Single "LongLong")                   = CppTypeDescription "long long int"          "long long int"           []
adjustTypeCpp (Single "ULongLong")                  = CppTypeDescription "unsigned long long int" "unsigned long long int"  []
adjustTypeCpp (Single "Bool")                       = CppTypeDescription "bool"                   "bool"                    []
adjustTypeCpp (Single "Float")                      = CppTypeDescription "double"                 "double"                  []
adjustTypeCpp (Single "LLVMSignedness")             = CppTypeDescription "llvm::Signedness"       "llvm::Signedness"        ["#include \"Util/util.h\""]
adjustTypeCpp (Single dt)                           = CppTypeDescription dtptr                    dtptr                     []
                                           where dtptr = dt ++ "::Ptr"
adjustTypeCpp (TTyApp (Single "[]") x)              = case adjustTypeCpp x of
                                                        CppTypeDescription storage _ includer ->
                                                          let stVector = "std::vector<" ++ storage ++ ">" in
                                                          CppTypeDescription
                                                            stVector
                                                            (cref stVector)
                                                            (["#include <vector>"] ++ includer)
adjustTypeCpp (TTyApp (Single "Maybe") x)           = case adjustTypeCpp x of
                                                        CppTypeDescription storage _ includer ->
                                                          let stOpt = "borealis::util::option<" ++ storage ++ ">" in
                                                          CppTypeDescription
                                                            stOpt
                                                            (cref stOpt)
                                                            (["#include \"Util/option.hpp\""] ++ includer)
adjustTypeCpp (TTyApp (TTyApp (Single "Map") k) v)  = case (adjustTypeCpp k, adjustTypeCpp v) of
                                                           (CppTypeDescription kstorage _ kincluder, CppTypeDescription vstorage _ vincluder) ->
                                                             let kvMap = "std::map<" ++ kstorage ++ ", " ++ vstorage ++ ">" in
                                                             CppTypeDescription
                                                               kvMap
                                                               (cref kvMap)
                                                               (["#include <map>"] ++ kincluder ++ vincluder)
adjustTypeCpp (TTyApp (Single "Exact") (Single x))  = CppTypeDescription x x []
adjustTypeCpp (TTyApp (Single "Param") (Single x))  = CppTypeDescription x (cref x) []
adjustTypeCpp wha                                   = error (show wha ++ ": unsupported generic type used")

data CppFieldDescriptor = CppFieldDescriptor{
    cppFieldName :: String,
    cppGetterName :: String,
    cppSetterName :: String,
    cppTypeDescription :: CppTypeDescription
} deriving (Show, Eq, Data, Typeable)

instance ToSElem CppFieldDescriptor where
  toSElem (CppFieldDescriptor field getter setter type_) = SM $ Data.Map.fromList [
      ("name", toSElem field),
      ("getter", toSElem getter),
      ("setter", toSElem setter),
      ("type", toSElem type_)
    ]

data CppDerivedDescriptor = CppDerivedDescriptor{
    cppConstructorName :: String,
    cppFields :: [CppFieldDescriptor]
} deriving (Show, Eq, Data, Typeable)

instance ToSElem CppDerivedDescriptor where
  toSElem (CppDerivedDescriptor name fields) = SM $ Data.Map.fromList [
      ("name", toSElem name),
      ("fields", toSElem fields)
    ]

makeGetterName (h:t) = "get" ++ [toUpper h] ++ t
makeSetterName (h:t) = "set" ++ [toUpper h] ++ t

adjustDescCpp (TDataTypeConstr x y) = CppDerivedDescriptor x $ map (\(a,b) -> CppFieldDescriptor a (makeGetterName a) (makeSetterName a) (adjustTypeCpp b)) y

-- end of C++ shit

-- protobuf shit

data ProtobufTypeDescription = ProtobufTypeDescription {
    pbufSpec :: String,
    pbufStoreName :: String,
    pbufLocalDefs :: [String],
    pbufImportStrings :: [String],
    pbufBuiltin :: Bool
} deriving (Show, Eq, Data, Typeable)

instance ToSElem ProtobufTypeDescription where
  toSElem (ProtobufTypeDescription spec store locals imports builtin) = SM $ Data.Map.fromList [
      ("spec", toSElem spec),
      ("name", toSElem store),
      ("locals", toSElem locals),
      ("imports", toSElem imports),
      ("builtin", toSElem builtin)
    ]

pbufDefaultNamespace = "borealis.proto"

collectionDescProto (ProtobufTypeDescription spec name locals _ builtin) | spec == "optional" || spec == "required" =
    unlines [
      "message " ++ name ++ "Collection {",
      unlines $ locals,
      "    repeated " ++ (if builtin then "" else (pbufDefaultNamespace ++ ".")) ++ name ++ " values = 1;",
      "};"
    ]
mapEntryDescProto (ProtobufTypeDescription kspec kname klocals _ kbuiltin) (ProtobufTypeDescription vspec vname vlocals _ vbuiltin) =
    unlines [
      "message " ++ kname ++ "2" ++ vname ++ "MapEntry {",
      unlines $ klocals,
      unlines $ vlocals,
      "    " ++ kspec ++ " " ++ (if kbuiltin then "" else (pbufDefaultNamespace ++ ".")) ++ kname ++ " key = 1;",
      "    " ++ vspec ++ " " ++ (if vbuiltin then "" else (pbufDefaultNamespace ++ ".")) ++ vname ++ " value = 2;",
      "};"
    ]

adjustTypeProto (Single "String")                    = ProtobufTypeDescription "optional" "string"     [] [] True
adjustTypeProto (Single "Int")                       = ProtobufTypeDescription "optional" "sint32"     [] [] True
adjustTypeProto (Single "UInt")                      = ProtobufTypeDescription "optional" "uint32"     [] [] True
adjustTypeProto (Single "Size")                      = ProtobufTypeDescription "optional" "uint32"     [] [] True
adjustTypeProto (Single "LongLong")                  = ProtobufTypeDescription "optional" "sint64"     [] [] True
adjustTypeProto (Single "ULongLong")                 = ProtobufTypeDescription "optional" "uint64"     [] [] True
adjustTypeProto (Single "Bool")                      = ProtobufTypeDescription "optional" "bool"       [] [] True
adjustTypeProto (Single "Float")                     = ProtobufTypeDescription "optional" "double"     [] [] True
adjustTypeProto (Single "LLVMSignedness")            = ProtobufTypeDescription "optional" "Signedness" [] ["import \"Util/Signedness.proto\";"] False
adjustTypeProto (Single dt)                          = ProtobufTypeDescription "optional" dt           [] [] False
{-- list case --}
adjustTypeProto (TTyApp (Single "[]") x)             = let ax = adjustTypeProto x in
                                                       case ax of
                                                         ProtobufTypeDescription pspec store pre includer builtin ->
                                                           case pspec of
                                                                x | x == "required" || x == "optional" ->
                                                                  ProtobufTypeDescription "repeated" store pre includer builtin
                                                                "repeated" ->
                                                                  ProtobufTypeDescription "repeated" (store ++ "Collection") ([collectionDescProto ax] ++ pre) includer True
adjustTypeProto (TTyApp (Single "Maybe") x)          = adjustTypeProto x
adjustTypeProto (TTyApp (TTyApp (Single "Map") k) v) = let ak = adjustTypeProto k; av = adjustTypeProto v in
                                                       case (ak, av) of
                                                         (ProtobufTypeDescription kspec kstore kpre kincluder kbuiltin, ProtobufTypeDescription vspec vstore vpre vincluder vbuiltin) ->
                                                           ProtobufTypeDescription
                                                             "repeated"
                                                             (kstore ++ "2" ++ vstore ++ "MapEntry")
                                                             ([mapEntryDescProto ak av] ++ kpre ++ vpre)
                                                             (kincluder ++ vincluder)
                                                             True
adjustTypeProto (TTyApp (Single "Exact") (Single x)) = ProtobufTypeDescription "optional" x     [] [] True
adjustTypeProto (TTyApp (Single "Param") (Single x)) = ProtobufTypeDescription "optional" x     [] [] True
adjustTypeProto wha                                  = error (show wha ++ ": unsupported generic type used")


data ProtobufFieldDescriptor = ProtobufFieldDescriptor{
    pbufFieldName :: String,
    pbufTypeDescription :: ProtobufTypeDescription
} deriving (Show, Eq, Data, Typeable)
instance ToSElem ProtobufFieldDescriptor where
  toSElem (ProtobufFieldDescriptor field type_) = SM $ Data.Map.fromList [
      ("name", toSElem field),
      ("type", toSElem type_)
    ]

data ProtobufDerivedDescriptor = ProtobufDerivedDescriptor{
    pbufConstructorName :: String,
    pbufFields :: [ProtobufFieldDescriptor]
} deriving (Show, Eq, Data, Typeable)

instance ToSElem ProtobufDerivedDescriptor where
  toSElem (ProtobufDerivedDescriptor name fields) = SM $ Data.Map.fromList [
      ("name", toSElem name),
      ("fields", toSElem fields)
    ]

adjustDescProtobuf (TDataTypeConstr x y) = ProtobufDerivedDescriptor x $ map (\(a,b) -> ProtobufFieldDescriptor a (adjustTypeProto b)) y

-- end of protobuf shit

data TwoExtensionFileName = TEFN{ thrine :: String, extension :: String, secondExtension :: String } deriving (Show, Eq)

tmplt <+= (k, v) = setAttribute k v tmplt

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


data CPlusPlus = CPlusPlus{ cppPrelude :: String, cppGuard :: String, cppConstructors :: [CppDerivedDescriptor], cppDescription :: (Maybe CppDerivedDescriptor) } deriving (Show, Eq)

instance ToSElem CPlusPlus where
  toSElem (CPlusPlus prelude guard constrs descr) = SM $ Data.Map.fromList [
      ("guard", toSElem guard),
      ("constructors", toSElem constrs),
      ("prelude", toSElem prelude),
      ("descriptor", toSElem descr)
    ]

data Protobuf = Protobuf{ protoPrelude :: String, protoLocals :: String, protoConstructors :: [ProtobufDerivedDescriptor], protoDescription :: (Maybe ProtobufDerivedDescriptor) } deriving (Show, Eq)

instance ToSElem Protobuf where
  toSElem (Protobuf prelude locals constrs descr) = SM $ Data.Map.fromList [
      ("prelude", toSElem prelude),
      ("locals", toSElem locals),
      ("constructors", toSElem constrs),
      ("descriptor", toSElem descr)
    ]

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
    let constructorsHere = map adjustDescCpp $ constrs datatype
    let constructorsProto = map adjustDescProtobuf $ constrs datatype

    flip mapM (template_bases) $ \ (template_base, fname) -> do
      let outExt = extension fname
      let outname = zipWithExt dir (dtname datatype, outExt)
      let include_guard = (map toUpper $ dtname datatype) ++ ext2guard outExt
      let filled_base :: StringTemplate String
          filled_base = template_base <+= ("dir", dir)
                                      <+= ("source_expr", dt_string)
                                      <+= ("output_file", outname)
                                      <+= ("source_file", source_file)
                                      <+= ("template_file", combine dir $ thrine fname <.> extension fname <.> secondExtension fname)
                                      <+= ("basename", dtname datatype)
                                      <+= ("cpp", CPlusPlus "" include_guard constructorsHere Nothing)
                                      <+= ("protobuf", Protobuf "" "" constructorsProto Nothing)
      writeFile outname $ render filled_base

    flip mapM (template_deriveds) $ \ (template_derived, fname) -> do
      flip mapM (zipWithIndex $ constrs datatype) $ \ (constr, ix) -> do
        let outExt = extension fname
        let outname = zipWithExt dir (tdconstrName constr, extension fname)
        let include_guard = (map toUpper $ tdconstrName constr) ++ ext2guard outExt
        let constructor = adjustDescCpp constr
        let constructorProto = adjustDescProtobuf constr
        let prelude = unlines $ nub $ concat $ flip map (cppFields constructor) $ \ it -> (cppImportStrings $ cppTypeDescription it)
        let protoPrelude = unlines $ nub $ concat $ flip map (pbufFields constructorProto) $ \ it -> (pbufImportStrings $ pbufTypeDescription it)
        let protoLocals = unlines $ nub $ concat $ flip map (pbufFields constructorProto) $ \ it -> (pbufLocalDefs $ pbufTypeDescription it)
        let filled_derived = template_derived <+= ("dir", dir)
                                              <+= ("source_expr", dt_string)
                                              <+= ("output_file", outname)
                                              <+= ("source_file", source_file)
                                              <+= ("template_file",combine dir $ thrine fname <.> extension fname <.> secondExtension fname)
                                              <+= ("basename", dtname datatype)
                                              <+= ("derivedname", cppConstructorName constructor)
                                              <+= ("index", ix+1)
                                              <+= ("index0", ix)
                                              <+= ("cpp", CPlusPlus prelude include_guard constructorsHere (Just constructor))
                                              <+= ("protobuf", Protobuf protoPrelude protoLocals constructorsProto (Just constructorProto))
        writeFile outname $ render filled_derived

    putStrLn "done!"
