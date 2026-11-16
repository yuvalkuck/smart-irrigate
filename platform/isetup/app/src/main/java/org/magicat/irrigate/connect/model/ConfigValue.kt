package org.magicat.irrigate.connect.model

import org.json.JSONObject

/** A single editable value from the device's status JSON, tagged with its original JSON type. */
sealed class ConfigValue {
    data class Text(val value: String) : ConfigValue()
    data class Number(val value: String) : ConfigValue()
    data class Bool(val value: Boolean) : ConfigValue()
}

data class ConfigField(val key: String, val value: ConfigValue)

/** Order-preserving: org.json's JSONObject keeps insertion order, matching the device's response. */
fun JSONObject.toConfigFields(): List<ConfigField> {
    val fields = mutableListOf<ConfigField>()
    val keys = keys()
    while (keys.hasNext()) {
        val key = keys.next()
        val raw = get(key)
        val value = when (raw) {
            is Boolean -> ConfigValue.Bool(raw)
            is kotlin.Number -> ConfigValue.Number(raw.toString())
            else -> ConfigValue.Text(if (raw == JSONObject.NULL) "" else raw.toString())
        }
        fields.add(ConfigField(key, value))
    }
    return fields
}

/** Renders as an INI document, e.g. "[setup]\nkey=value\n...", for POSTing to /api/set. */
fun List<ConfigField>.toIniString(section: String = "setup"): String = buildString {
    append('[').append(section).append("]\n")
    for (field in this@toIniString) {
        val raw = when (val value = field.value) {
            is ConfigValue.Text -> value.value
            is ConfigValue.Number -> value.value
            is ConfigValue.Bool -> value.value.toString()
        }
        append(field.key).append('=').append(raw).append('\n')
    }
}
