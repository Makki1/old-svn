#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_Constants.py
#Version: V0.1 , 06.01.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

##contains all used constants in FB-ETS



#MainNodes of XML-Data File to create a new Data File
#all SubNodes were added from all versions of ETS-Files
ManufacturerNode = ["manufacturer","MANUFACTURER_ID",
                                   "MANUFACTURER_NAME"]

FunctionalNode =   ["functional_entity","FUNCTIONAL_ENTITY_ID",
                                        "MANUFACTURER_ID",
                                        "FUNCTIONAL_ENTITY_NAME",
                                        "FUNCTIONAL_ENTITY_NUMB"]

BCUNode =          ["bcu_type","BCU_TYPE_NUMBER",
                               "BCU_TYPE_NAME",
                               "BCU_TYPE_CPU"]

SymbolNode =       ["symbol","SYMBOL_ID",
                             "SYMBOL_FILENAME",
                             "SYMBOL_NAME",
                             "SYMBOL_DATA",
                             "IconData"]

ProductNode =      ["hw_product","PRODUCT_ID",
                                 "MANUFACTURER_ID",
                                 "SYMBOL_ID",
                                 "PRODUCT_NAME",
                                 "PRODUCT_VERSION_NUMBER",
                                 "COMPONENT_TYPE",
                                 "COMPONENT_ATTRIBUTES",
                                 "BUS_CURRENT",
                                 "PRODUCT_SERIAL_NUMBER",
                                 "COMPONENT_TYPE_NUMBER",
                                 "PRODUCT_PICTURE",
                                 "BCU_TYPE_NUMBER",
                                 "PRODUCT_HANDLING",
                                 "PRODUCT_DLL",
                                 "ORIGINAL_MANUFACTURER_ID"]

CatalogNode =      ["catalog_entry","CATALOG_ENTRY_ID",
                                    "PRODUCT_ID",
                                    "MANUFACTURER_ID",
                                    "ORDER_NUMBER",
                                    "ENTRY_NAME",
                                    "DIN_FLAG",
                                    "PAGE_NUMBER",
                                    "DESIGNATION_TYPE",
                                    "ENTRY_STATUS_CODE",
                                    "REGISTRATION_TS"]
MediumNode =       ["medium_type","MEDIUM_TYPE_NUMBER",
                                  "MEDIUM_TYPE_NAME",
                                  "MEDIUM_TYPE_SHORT_NAME"]

MaskNode =         ["mask","MASK_ID",
                           "MASK_VERSION",
                           "USER_RAM_START",
                           "USER_RAM_END",
                           "USER_EEPROM_START",
                           "USER_EEPROM_END",
                           "RUN_ERROR_ADDRESS",
                           "ADDRESS_TAB_ADDRESS",
                           "ASSOCTABPTR_ADDRESS",
                           "COMMSTABPTR_ADDRESS",
                           "MANUFACTURER_DATA_ADDRESS",
                           "MANUFACTURER_DATA_SIZE",
                           "MANUFACTURER_ID_ADDRESS",
                           "ROUTECNT_ADDRESS",
                           "MANUFACTURER_ID_PROTECTED",
                           "MASK_VERSION_NAME",
                           "MASK_EEPROM_DATA",
                           "MASK_DATA_LENGTH",
                           "ADDRESS_TAB_LCS",
                           "ASSOC_TAB_LCS",
                           "APPLICATION_PROGRAM_LCS",
                           "PEI_PROGRAM_LCS",
                           "LOAD_CONTROL_ADDRESS",
                           "RUN_CONTROL_ADDRESS",
                           "PORT_ADDRESS_PROTECTED",
                           "MEDIUM_TYPE_NUMBER",
                           "BCU_TYPE_NUMBER"]

AppNode =          ["application_program","PROGRAM_ID",
                                          "SYMBOL_ID",
                                          "MASK_ID",
                                          "PROGRAM_NAME",
                                          "PROGRAM_VERSION",
                                          "PROGRAM_VERSION_NUMBER",
                                          "LINKABLE",
                                          "DEVICE_TYPE",
                                          "PEI_TYPE",
                                          "ADDRESS_TAB_SIZE",
                                          "ASSOCTAB_ADDRESS",
                                          "ASSOCTAB_SIZE",
                                          "COMMSTAB_ADDRESS",
                                          "COMMSTAB_SIZE",
                                          "PROGRAM_SERIAL_NUMBER",
                                          "MANUFACTURER_ID",
                                          "EEPROM_DATA",
                                          "DATA_LENGTH",
                                          "S19_FILE",
                                          "MAP_FILE",
                                          "ASSEMBLER_ID",
                                          "HELP_FILE_NAME",
                                          "CONTEXT_ID",
                                          "DYNAMIC_MANAGEMENT",
                                          "PROGRAM_TYPE",
                                          "RAM_SIZE",
                                          "ORIGINAL_MANUFACTURER_ID",
                                          "API_VERSION",
                                          "PROGRAM_STYLE",
                                          "IS_POLLING_MASTER",
                                          "NUMBER_OF_POLLING_GROUPS",
                                          "AllowedInSimpleEts",
                                          "MinEtsVersion"]

VirDeviceNode =    ["virtual_device","VIRTUAL_DEVICE_ID",
                                     "SYMBOL_ID",
                                     "CATALOG_ENTRY_ID",
                                     "PROGRAM_ID",
                                     "VIRTUAL_DEVICE_NAME",
                                     "FUNCTIONAL_ENTITY_ID",
                                     "PRODUCT_TYPE_ID",
                                     "VIRTUAL_DEVICE_NUMBER",
                                     "MEDIUM_TYPES"]

DeviceInfoNode =   ["device_info","DEVICE_INFO_ID",
                                  "MASK_ID",
                                  "DEVICE_INFO_NAME",
                                  "DEVICE_INFO_VISIBLE"]
eteLangNode =      ["ete_language","LANGUAGE_ID",
                                   "LANGUAGE_NAME",
                                   "DATABASE_LANGUAGE"]
DeviceInfoVNode =  ["device_info_value","DEVICE_INFO_VALUE_ID",
                                        "DEVICE_INFO_ID",
                                        "BITMAP_ID"]
ParaAtomTypeNode = ["parameter_atomic_type","ATOMIC_TYPE_NUMBER",
                                            "ATOMIC_TYPE_NAME",
                                            "DISPATTR"]
ParaTypeNode =     ["parameter_type","PARAMETER_TYPE_ID",
                                     "ATOMIC_TYPE_NUMBER",
                                     "PROGRAM_ID",
                                     "PARAMETER_TYPE_NAME",
                                     "PARAMETER_TYPE_LOW_ACCESS",
                                     "PARAMETER_TYPE_HIGH_ACCESS",
                                     "PARAMETER_TYPE_SIZE"]
ParaListVNode =    ["parameter_list_of_values","PARAMETER_TYPE_ID",
                                               "REAL_VALUE",
                                               "DISPLAYED_VALUE",
                                               "DISPLAY_ORDER",
                                               "PARAMETER_VALUE_ID",
                                               "BINARY_VALUE_LENGTH"]
ParaNode =         ["parameter","PROGRAM_ID",
                                "PARAMETER_TYPE_ID",
                                "PARAMETER_NUMBER",
                                "PARAMETER_NAME",
                                "PARAMETER_LOW_ACCESS",
                                "PARAMETER_HIGH_ACCESS",
                                "PARAMETER_SIZE",
                                "PARAMETER_DISPLAY_ORDER",
                                "PARAMETER_ADDRESS",
                                "PARAMETER_BITOFFSET",
                                "PARAMETER_DESCRIPTION",
                                "PARAMETER_ID",
                                "PAR_PARAMETER_ID",
                                "PARAMETER_DEFAULT_LONG",
                                "PATCH_ALWAYS",
                                "ADDRESS_SPACE"]
ObjectTypeNode =   ["object_type","OBJECT_TYPE_CODE",
                                  "OBJECT_TYPE_NAME",
                                  "LENGTH_IN_BIT"]
ObjectPrioNode =   ["object_priority","OBJECT_PRIORITY_CODE",
                                      "OBJECT_PRIORITY_NAME"]
CommObjNode =      ["communication_object","PROGRAM_ID",
                                           "OBJECT_NAME",
                                           "OBJECT_FUNCTION",
                                           "OBJECT_READENABLED",
                                           "OBJECT_WRITEENABLED",
                                           "OBJECT_COMMENABLED",
                                           "OBJECT_TRANSENABLED",
                                           "OBJECT_DISPLAY_ORDER",
                                           "PARENT_PARAMETER_VALUE",
                                           "OBJECT_ID",
                                           "PARAMETER_ID",
                                           "OBJECT_NUMBER",
                                           "OBJECT_TYPE",
                                           "OBJECT_PRIORITY",
                                           "OBJECT_UPDATEENABLED",
                                           "OBJECT_UNIQUE_NUMBER"]
TextNode =         ["text_attribute","TEXT_ATTRIBUTE_ID",
                                     "LANGUAGE_ID",
                                     "COLUMN_ID",
                                     "ENTITY_ID",
                                     "TEXT_ATTRIBUTE_TEXT"]
ProdDescrNode =    ["product_description","PRODUCT_DESCRIPTION_ID",
                                          "CATALOG_ENTRY_ID",
                                          "DISPLAY_ORDER",
                                          "LANGUAGE_ID"]
HelpFileNode =     ["help_file","HELP_FILE_ID",
                                "LANGUAGE_ID",
                                "ENTITY_ID"]
Prod2ProgrNode =   ["product_to_program","PROD2PROG_ID",
                                         "PRODUCT_ID",
                                         "PROGRAM_ID",
                                         "PROD2PROG_STATUS_CODE",
                                         "REGISTRATION_NUMBER",
                                         "REGISTRATION_YEAR",
                                         "ORIGINAL_REGISTRATION_NUMBER",
                                         "ORIGINAL_REGISTRATION_YEAR",
                                         "REGISTRATION_TS"]
Prod2ProgrMTNode = ["product_to_program_to_mt","PROD2PROG2MT_ID",
                                               "PROD2PROG_ID",
                                               "MEDIUM_TYPE_NUMBER"]
ProgrDescrNode =   ["program_description","PROGRAM_DESCRIPTION_ID",
                                          "PROGRAM_ID",
                                          "DISPLAY_ORDER",
                                          "LANGUAGE_ID"]
S19BlockNode =     ["s19_block","BLOCK_ID",
                                "BLOCK_NUMBER",
                                "PROGRAM_ID",
                                "BLOCK_NAME",
                                "CONTROL_CODE"]
S19BlockParaNode = ["s19_block_paragraph","S19_BLOCK_PARAGRAPH_ID",
                                          "BLOCK_ID",
                                          "PT_COLUMN_ID",
                                          "DATA_LONG",
                                          "DATA_BINARY"]
DeviceParaNode =   ["device_parameter","DEVICE_ID",
                                       "DEVICE_PARAMETER_ID"
                                       "PARAMETER_ID",
                                       "DEVICE_PARAMETER_NUMBER",
                                       "DEVICE_PARAMETER_VISIBLE",
                                       "PARAMETER_VALUE_LONG",
                                       "PROGRAM_TYPE",
                                       "ValueIsValid"]
DeviceObjNode =    ["device_object","DEVICE_ID",
                                    "OBJECT_PRIO",
                                    "OBJECT_READ",
                                    "OBJECT_WRITE",
                                    "OBJECT_COMM",
                                    "OBJECT_TRANS",
                                    "DEVICE_OBJECT_ID",
                                    "OBJECT_ID",
                                    "DEVICE_OBJECT_NUMBER",
                                    "DEVICE_OBJECT_VISIBLE",
                                    "DEVICE_OBJECT_UNIQUE_NAME",
                                    "OBJECT_UPDATE",
                                    "DEVICE_OBJECT_UNIQUE_NUMBER",
                                    "DEVICE_OBJECT_TYPE"]
maskFeatureNode =  ["mask_feature","MASK_FEATURE_ID",
                                   "MASK_FEATURE_NAME",]
mask2maskNode =    ["mask_to_mask_feature","MASK_TO_MASK_FEATURE_ID",
                                           "MASK_ID",
                                           "MASK_FEATURE_ID"]

#-----------------------------------------------------------------------

