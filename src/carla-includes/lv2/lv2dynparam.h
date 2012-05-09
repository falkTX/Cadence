/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *  This work is in public domain.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  If you have questions, contact Nedko Arnaudov <nedko@arnaudov.name> or
 *  ask in #lad channel, FreeNode IRC network.
 *
 *****************************************************************************/

/**
 * @file lv2dynparam.h
 * @brief LV2 dynparam extension definition
 *
 * @par Purpose
 *
 * The purpose of this extension is to allow plugin parameters appear
 * and disappear as response of existing parameter changes (i.e. number
 * of voices) or executing commands (i.e. "add new voice"). It also
 * allows grouping of parameters and groups. Groups can be used for
 * things like ADSR abstraction, i.e. group of 4 float parameters.
 *
 * @par Architectural overview
 *
 * Plugin notifies host for changes through callbacks. During
 * initialization, plugin notifies host about initial groups,
 * parameters and commands through same callbacks used for later
 * notification. There are callbacks to notify host for group,
 * parameter and command disappears.
 *
 * Groups are containers of other groups, parameters and
 * commands. Parameters and groups have URIs. Parameter URIs are
 * used to describe type of parameter (i.e. boolean, integer,
 * string, etc.). Parameters are as simple as possible. There is one
 * predefined Group URI for "generic group" type, i.e. container
 * that is just that - a container of other groups, parameters and
 * commands. Other group types are just hints and ones that are
 * unknown to host, can be looked as generic ones. Only generic
 * groups can contain groups. Groups of other types can contain only
 * parameters. There is always one, root group. Name of the root
 * groop is expected to match name of the plugin.
 *
 * Groups, parameters and commands, have "name" containing short
 * human readble description of object purpose.
 *
 * Parameter ports, are bidirectional. Parameters have values, and
 * some of them (depending of type) - range. Data storage for
 * current, min and max values is in plugin memory. Host gets
 * pointers to that memory and accesses the data in type specific
 * way.
 *
 * When plugin decides to change parameter value or range (as
 * response to command execution or other parameter value change), it
 * notifies host through callback.
 *
 * When host decides to change parameter value or execute command (as
 * resoponse to user action, i.e. knob rotation/button press, or some
 * kind of automation), it calls plugin callback. In case of
 * parameter, it first changes the value in the plugin data storage
 * for particular parameter. Host ensures that values being set are
 * within current range (if range is defined for particular port
 * type).
 *
 * Apart from initialization (host_attach), plugin may call host
 * callbacks only as response to command execution or parameter
 * change notification (from host).
 *
 * Host serializes calls to plugin dynparam callbacks and the
 * run/connect_port lv2 callbacks. Thus plugin does not need to take
 * special measures for thread safety.  in callbacks called by host.
 *
 * Plugin can assume that host will never call dynamic parameter
 * functions when lv2 plugin callbacks (like run and connect_port) are
 * being called. I.e. calls are serialized, no need for locks in
 * plugin. Plugin must not suspend execution (sleep/lock) while being
 * called by host. If it needs to sleep to implement action requested
 * by host, like allocation of data for new voice, the allocation
 * should be done in background thread and when ready, transfered to
 * the realtime code. During initialization (host_attach) plugin is
 * allowed to sleep. Thus conventional memory allocation is allowed
 * in host_attach.
 *
 * @par Intialization sequence
 *
 * -# Host discovers whether plugin supports the extension by
 *    inspecting the RDF Turtle file.
 * -# Host calls lv2 extension_data callback, plugin returns pointer
 *    to struct lv2dynparam_plugin_callbacks containing pointers to
 *    several funtions, including host_attach.
 * -# For each instantiated plugin supporting this extension, host
 *    calls host_attach function with parameters:
 *    - a LV2_Handle, plugin instance handle
 *    - pointer to struct lv2dynparam_host_callbacks, containing
 *      pointers to several functions, to be called by plugin.
 *    - instance host context, opaque pointer
 * -# During initialization (host_attach), initial groups and
 *    parameters appear and host callbacks are called by plugin.
 *
 * @par Parameter types
 * - float, with range
 * - 32-bit 2's complement signed int, with range
 * - midi note, value stored as byte, with range
 * - string
 * - filename
 * - boolean, value stored as byte, non-zero means TRUE, zero means FALSE 
 *
 * @par Notes:
 * - if function fails, variables where output parameters are normally
 *   stored remain unmodified.
 * - If host callback, called by plugin, fails - plugin should try the
 *   operation later.
 */


#ifndef LV2DYNPARAM_H__31DEB371_3874_44A0_A9F2_AAFB0360D8C5__INCLUDED
#define LV2DYNPARAM_H__31DEB371_3874_44A0_A9F2_AAFB0360D8C5__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

/** base URI to be used for composing real URIs (internal use only) */
#define LV2DYNPARAM_BASE_URI "http://home.gna.org/lv2dynparam/v1"

/** URI of the LV2 extension defined in this file */
#define LV2DYNPARAM_URI LV2DYNPARAM_BASE_URI

/** max size of name and type_uri buffers, including terminating zero char */
#define LV2DYNPARAM_MAX_STRING_SIZE 1024

/** handle identifying parameter, supplied by plugin */
typedef void * lv2dynparam_parameter_handle;

/** handle identifying group, supplied by plugin */
typedef void * lv2dynparam_group_handle;

/** handle identifying command, supplied by plugin */
typedef void * lv2dynparam_command_handle;

/**
 * Structure containing set of hints.
 */
struct lv2dynparam_hints
{
  unsigned char count;          /**< Number of hints in the set. Size of @c names and @c values arrays. */
  char ** names;                /**< Hint names. Array of hint names with size @c count. */
  char ** values;               /**< Hint values. Array of hint values with size @c count. Element can be NULL if hint has no value. */
};

/**
 * Structure containing poitners to functions called by
 * plugin and implemented by host.
 */
struct lv2dynparam_host_callbacks
{
  /**
   * This function is called by plugin to notify host about new
   * group.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param parent_group_host_context Host context of parent
   * group. For the root group, parent context is NULL.
   * @param group Plugin context for the group
   * @param hints_ptr Pointer to structure representing hints set
   * associated with appearing group.
   * @param group_host_context Pointer to variable where host context
   * for the new group will be stored.
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*group_appear)(
    void * instance_host_context,
    void * parent_group_host_context,
    lv2dynparam_group_handle group,
    const struct lv2dynparam_hints * hints_ptr,
    void ** group_host_context);

  /**
   * This function is called by plugin to notify host about group
   * removal.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param group_host_context Host context of the group
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*group_disappear)(
    void * instance_host_context,
    void * group_host_context);

  /**
   * This function is called by plugin to notify host about new
   * parameter.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param group_host_context Host context of parent group.
   * @param parameter Plugin context for the parameter
   * @param hints_ptr Pointer to structure representing hints set
   * associated with appearing parameter.
   * @param parameter_host_context Pointer to variable where host
   * context for the new parameter will be stored.
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*parameter_appear)(
    void * instance_host_context,
    void * group_host_context,
    lv2dynparam_parameter_handle parameter,
    const struct lv2dynparam_hints * hints_ptr,
    void ** parameter_host_context);

  /**
   * This function is called by plugin to notify host about parameter
   * removal.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param parameter_host_context Host context of the parameter
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*parameter_disappear)(
    void * instance_host_context,
    void * parameter_host_context);

  /**
   * This function is called by plugin to notify host about parameter
   * value or range change.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param parameter_host_context Host context of the parameter
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*parameter_change)(
    void * instance_host_context,
    void * parameter_host_context);

  /**
   * This function is called by plugin to notify host about new commmand.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param group_host_context Host context of parent group.
   * @param command Plugin context for the command
   * @param hints_ptr Pointer to structure representing hints set
   * associated with appearing command.
   * @param command_host_context Pointer to variable where host
   * context for the new command will be stored.
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*command_appear)(
    void * instance_host_context,
    void * group_host_context,
    lv2dynparam_command_handle command,
    const struct lv2dynparam_hints * hints_ptr,
    void ** command_host_context);

  /**
   * This function is called by plugin to notify host about command
   * removal.
   *
   * @param instance_host_context Host instance context, as supplied
   * during initialization (host_attach).
   * @param command_host_context Host context of the command
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*command_disappear)(
    void * instance_host_context,
    void * command_host_context);
};

/**
 * Structure containing poitners to functions called by
 * host and implemented by plugin.
 * Pointer to this struct is returned by LV2 extension_data plugin callback
 */
struct lv2dynparam_plugin_callbacks
{
  /**
   * This callback is called by host during initialization.
   * Plugin is allowed to suspend execution (sleep/lock).
   * Thus conventional memory allocation is allowed.
   *
   * @param instance Handle of instantiated LV2 plugin
   * @param host_callbacks Pointer to struncture containing pointer to
   * functions implemented by host, to be called by plugin.
   * @param instance_host_context Context to be supplied as parameter
   * to callbacks implemented by host and called by plugin.
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error
   */
  unsigned char (*host_attach)(
    LV2_Handle instance,
    struct lv2dynparam_host_callbacks * host_callbacks,
    void * instance_host_context);

  /**
   * This function is called by host to retrieve name of group
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param group Group handle, as supplied by plugin
   * @param buffer Pointer to buffer with size of
   * LV2DYNPARAM_MAX_STRING_SIZE bytes, where ASCIIZ string containing
   * name of the group will be stored.
   */
  void (*group_get_name)(
    lv2dynparam_group_handle group,
    char * buffer);

  /**
   * This function is called by host to retrieve type URI of parameter
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param parameter Parameter handle, as supplied by plugin
   * @param buffer Pointer to buffer with size of
   * LV2DYNPARAM_MAX_STRING_SIZE bytes, where ASCIIZ string containing
   * type URI of the parameter will be stored.
   */
  void (*parameter_get_type_uri)(
    lv2dynparam_parameter_handle parameter,
    char * buffer);

  /**
   * This function is called by host to retrieve name of parameter
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param parameter Parameter handle, as supplied by plugin
   * @param buffer Pointer to buffer with size of
   * LV2DYNPARAM_MAX_STRING_SIZE bytes, where ASCIIZ string containing
   * name of the parameter will be stored.
   */
  void (*parameter_get_name)(
    lv2dynparam_parameter_handle parameter,
    char * buffer);

  /**
   * This funtion is called by host to retrieve pointer to memory
   * where value data for particular parameter is stored.
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param parameter Parameter handle, as supplied by plugin
   * @param value_buffer Pointer to variable where pointer to value
   * data will be stored.
   */
  void (*parameter_get_value)(
    lv2dynparam_parameter_handle parameter,
    void ** value_buffer);

  /**
   * This funtion is called by host to retrieve pointer to memory
   * where range min and max values data for particular parameter are
   * stored. Host calls this function only for parameter with types
   * that have range.
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param parameter Parameter handle, as supplied by plugin
   * @param value_min_buffer Pointer to variable where pointer to min
   * value data will be stored.
   * @param value_max_buffer Pointer to variable where pointer to max
   * value data will be stored.
   */
  void (*parameter_get_range)(
    lv2dynparam_parameter_handle parameter,
    void ** value_min_buffer,
    void ** value_max_buffer);

  /**
   * This function is called by host to notify plugin about value
   * change. For parameters with types that have range, value is
   * guaranteed to be with the range. Before calling this function,
   * host updates value data storage with the new value.
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param parameter Parameter handle, as supplied by plugin
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*parameter_change)(
    lv2dynparam_parameter_handle parameter);

  /**
   * This function is called by host to retrieve name of command
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param command Command handle, as supplied by plugin
   * @param buffer Pointer to buffer with size of
   * LV2DYNPARAM_MAX_STRING_SIZE bytes, where ASCIIZ string containing
   * name of the command will be stored.
   */
  void (*command_get_name)(
    lv2dynparam_command_handle command,
    char * buffer);

  /**
   * This function is called by host to execute command defined by
   * plugin.
   * Plugin implementation must not suspend execution (sleep/lock).
   *
   * @param command Command handle, as supplied by plugin
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error, try later
   */
  unsigned char (*command_execute)(
    lv2dynparam_command_handle command);
};

/** URI for float parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_FLOAT_URI         LV2DYNPARAM_BASE_URI "#parameter_float"

/** URI for integer parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_INT_URI           LV2DYNPARAM_BASE_URI "#parameter_int"

/** URI for note parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_NOTE_URI          LV2DYNPARAM_BASE_URI "#parameter_note"

/** URI for string parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_STRING_URI        LV2DYNPARAM_BASE_URI "#parameter_string"

/** URI for filename parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_FILENAME_URI      LV2DYNPARAM_BASE_URI "#parameter_filename"

/** URI for boolean parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_BOOLEAN_URI       LV2DYNPARAM_BASE_URI "#parameter_boolean"

/** URI for enumeration parameter */
#define LV2DYNPARAM_PARAMETER_TYPE_ENUM_URI          LV2DYNPARAM_BASE_URI "#parameter_enum"

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef LV2DYNPARAM_H__31DEB371_3874_44A0_A9F2_AAFB0360D8C5__INCLUDED */
