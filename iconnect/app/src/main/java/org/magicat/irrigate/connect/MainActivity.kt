package org.magicat.irrigate.connect

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch
import org.magicat.irrigate.connect.model.ConfigField
import org.magicat.irrigate.connect.model.ConfigValue
import org.magicat.irrigate.connect.model.toConfigFields
import org.magicat.irrigate.connect.model.toIniString
import org.magicat.irrigate.connect.network.DeviceApi
import org.magicat.irrigate.connect.ui.theme.IrrigateConnectTheme

/** Background behind every setting field's text. */
private val FieldBackgroundColor = Color.Black

/** Text color when the value still matches what the device last reported. */
private val UnmodifiedTextColor = Color.White

/** Text color when the value no longer matches what the device last reported. */
private val ModifiedTextColor = Color.Yellow

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            IrrigateConnectTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    MainScreen(modifier = Modifier.padding(innerPadding))
                }
            }
        }
    }
}

@Composable
fun MainScreen(modifier: Modifier = Modifier) {
    val host = BuildConfig.DEVICE_HOST
    val port = BuildConfig.DEVICE_PORT.toString()
    var fields by remember { mutableStateOf<List<ConfigField>>(emptyList()) }
    // Baseline snapshot from the device, used to detect which fields the user has edited.
    var baseline by remember { mutableStateOf<Map<String, ConfigValue>>(emptyMap()) }
    var isLoading by remember { mutableStateOf(false) }
    var statusMessage by remember { mutableStateOf<String?>(null) }
    var isError by remember { mutableStateOf(false) }
    var restartOnApply by remember { mutableStateOf(true) }

    val scope = rememberCoroutineScope()

    fun baseUrl() = "http://${host.trim()}:${port.trim()}"

    fun connect(silent: Boolean = false) {
        if (!silent) {
            statusMessage = null
            isError = false
        }
        isLoading = true
        scope.launch {
            try {
                val json = DeviceApi.fetchStatus(baseUrl())
                val loaded = json.toConfigFields()
                fields = loaded
                baseline = loaded.associate { it.key to it.value }
                statusMessage = "Loaded ${loaded.size} setting(s)"
                isError = false
            } catch (e: Exception) {
                // On the silent startup probe, just fall back to the "connect to device" button
                // instead of surfacing an error the user never asked for.
                if (!silent) {
                    isError = true
                    statusMessage = "Failed to load status: ${e.message}"
                }
            } finally {
                isLoading = false
            }
        }
    }

    fun set() {
        statusMessage = null
        isError = false
        isLoading = true
        val requested = fields
        scope.launch {
            try {
                DeviceApi.pushStatus(baseUrl(), requested.toIniString())
                val refreshed = DeviceApi.fetchStatus(baseUrl()).toConfigFields()
                fields = refreshed
                baseline = refreshed.associate { it.key to it.value }
                val refreshedByKey = refreshed.associate { it.key to it.value }
                val mismatched = requested.filter { refreshedByKey[it.key] != it.value }
                if (mismatched.isEmpty()) {
                    statusMessage = "Settings applied and verified"
                } else {
                    isError = true
                    statusMessage = "Applied, but device did not accept: ${mismatched.joinToString { it.key }}"
                }
                if (restartOnApply) {
                    try {
                        DeviceApi.restartDevice(baseUrl())
                        statusMessage = "$statusMessage — device restart requested"
                    } catch (e: Exception) {
                        statusMessage = "$statusMessage — restart request failed: ${e.message}"
                    }
                }
            } catch (e: Exception) {
                isError = true
                statusMessage = "Failed to apply settings: ${e.message}"
            } finally {
                isLoading = false
            }
        }
    }

    fun updateField(key: String, newValue: ConfigValue) {
        fields = fields.map { if (it.key == key) it.copy(value = newValue) else it }
    }

    // Probe the device once on launch; if it's not reachable we just fall back to the button.
    LaunchedEffect(Unit) { connect(silent = true) }

    LazyColumn(
        modifier = modifier
            .fillMaxWidth()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        item {
            Text("Device connection", style = MaterialTheme.typography.titleMedium)
        }
        item {
            Text("$host:$port", style = MaterialTheme.typography.bodyLarge)
        }
        item {
            Button(onClick = { connect() }, enabled = !isLoading) {
                Text(if (fields.isEmpty()) "connect to device" else "Refresh")
            }
        }

        if (fields.isNotEmpty()) {
            item { HorizontalDivider() }
            item { Text("Settings", style = MaterialTheme.typography.titleMedium) }
            items(fields, key = { it.key }) { field ->
                ConfigFieldRow(
                    field = field,
                    isModified = baseline[field.key] != field.value,
                    onValueChange = { updateField(field.key, it) },
                )
            }
            item {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(
                        checked = restartOnApply,
                        onCheckedChange = { restartOnApply = it },
                    )
                    Text("Restart device on apply")
                }
            }
            item {
                Button(onClick = { set() }, enabled = !isLoading) {
                    Text("Apply")
                }
            }
        }

        item {
            if (isLoading) {
                CircularProgressIndicator()
            }
            statusMessage?.let {
                Text(
                    it,
                    color = if (isError) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.primary,
                )
            }
        }
    }
}

@Composable
private fun ConfigFieldRow(
    field: ConfigField,
    isModified: Boolean,
    onValueChange: (ConfigValue) -> Unit,
) {
    val backgroundColor = FieldBackgroundColor
    val contentColor = if (isModified) ModifiedTextColor else UnmodifiedTextColor

    when (val value = field.value) {
        is ConfigValue.Bool -> {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(backgroundColor, RoundedCornerShape(8.dp))
                    .padding(horizontal = 16.dp, vertical = 12.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
            ) {
                Text(field.key, color = contentColor, modifier = Modifier.weight(1f))
                Switch(
                    checked = value.value,
                    onCheckedChange = { onValueChange(ConfigValue.Bool(it)) },
                )
            }
        }
        is ConfigValue.Number -> {
            TextField(
                value = value.value,
                onValueChange = { onValueChange(ConfigValue.Number(it)) },
                label = { Text(field.key) },
                singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal),
                colors = fieldColors(backgroundColor, contentColor),
                modifier = Modifier.fillMaxWidth(),
            )
        }
        is ConfigValue.Text -> {
            TextField(
                value = value.value,
                onValueChange = { onValueChange(ConfigValue.Text(it)) },
                label = { Text(field.key) },
                singleLine = true,
                colors = fieldColors(backgroundColor, contentColor),
                modifier = Modifier.fillMaxWidth(),
            )
        }
    }
}

@Composable
private fun fieldColors(backgroundColor: Color, contentColor: Color) = TextFieldDefaults.colors(
    focusedContainerColor = backgroundColor,
    unfocusedContainerColor = backgroundColor,
    disabledContainerColor = backgroundColor,
    focusedTextColor = contentColor,
    unfocusedTextColor = contentColor,
    focusedLabelColor = contentColor,
    unfocusedLabelColor = contentColor,
    cursorColor = contentColor,
)

@Preview(showBackground = true)
@Composable
fun MainScreenPreview() {
    IrrigateConnectTheme {
        MainScreen()
    }
}
