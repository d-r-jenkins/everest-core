// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include "evse_managerImpl.hpp"
#include <utils/date.hpp>

#include <date/date.h>
#include <date/tz.h>
#include <utils/date.hpp>

#include <fmt/core.h>

#include "SessionLog.hpp"

namespace module {

namespace evse {

bool str_to_bool(const std::string& data) {
    if (data == "true") {
        return true;
    }
    return false;
}

void evse_managerImpl::init() {
    limits.nr_of_phases_available = 1;
    limits.max_current = 0.;

    mod->signalNrOfPhasesAvailable.connect([this](const int n) {
        if (n >= 1 && n <= 3) {
            limits.nr_of_phases_available = n;
            publish_limits(limits);
        }
    });

    // Interface to Node-RED debug UI

    mod->mqtt.subscribe(
        fmt::format("everest_external/nodered/{}/cmd/set_max_current", mod->config.connector_id),
        [&charger = mod->charger, this](std::string data) { mod->nodered_set_current_limit(std::stof(data)); });

    mod->mqtt.subscribe(
        fmt::format("everest_external/nodered/{}/cmd/set_max_watt", mod->config.connector_id),
        [&charger = mod->charger, this](std::string data) { mod->nodered_set_watt_limit(std::stof(data)); });

    mod->mqtt.subscribe(fmt::format("everest_external/nodered/{}/cmd/enable", mod->config.connector_id),
                        [&charger = mod->charger](const std::string& data) {
                            charger->enable_disable(0, {types::evse_manager::Enable_source::LocalAPI,
                                                        types::evse_manager::Enable_state::Enable, 100});
                        });

    mod->mqtt.subscribe(fmt::format("everest_external/nodered/{}/cmd/disable", mod->config.connector_id),
                        [&charger = mod->charger](const std::string& data) {
                            charger->enable_disable(0, {types::evse_manager::Enable_source::LocalAPI,
                                                        types::evse_manager::Enable_state::Disable, 100});
                        });

    mod->mqtt.subscribe(fmt::format("everest_external/nodered/{}/cmd/faulted", mod->config.connector_id),
                        [&charger = mod->charger](const std::string& data) { charger->set_faulted(); });

    mod->mqtt.subscribe(
        fmt::format("everest_external/nodered/{}/cmd/switch_three_phases_while_charging", mod->config.connector_id),
        [&charger = mod->charger](const std::string& data) {
            charger->switch_three_phases_while_charging(str_to_bool(data));
        });

    mod->mqtt.subscribe(fmt::format("everest_external/nodered/{}/cmd/pause_charging", mod->config.connector_id),
                        [&charger = mod->charger](const std::string& data) { charger->pause_charging(); });

    mod->mqtt.subscribe(fmt::format("everest_external/nodered/{}/cmd/resume_charging", mod->config.connector_id),
                        [&charger = mod->charger](const std::string& data) { charger->resume_charging(); });

    // /Interface to Node-RED debug UI

    if (mod->r_powermeter_billing().size() > 0) {
        mod->r_powermeter_billing()[0]->subscribe_powermeter([this](const types::powermeter::Powermeter& p) {
            // Republish data on proxy powermeter struct
            publish_powermeter(p);
        });
        mod->r_powermeter_billing()[0]->subscribe_public_key_ocmf([this](const std::string& public_key_ocmf) {
            // Republish data on proxy powermeter public_key_ocmf
            publish_powermeter_public_key_ocmf(public_key_ocmf);
        });
    }
}

void evse_managerImpl::ready() {

    // publish evse id at least once
    publish_evse_id(mod->config.evse_id);

    mod->r_bsp->subscribe_telemetry([this](types::evse_board_support::Telemetry telemetry) {
        // external Nodered interface
        mod->mqtt.publish(fmt::format("everest_external/nodered/{}/state/temperature", mod->config.connector_id),
                          telemetry.evse_temperature_C);
        // external Nodered interface
        publish_telemetry(telemetry);
    });

    // The module code generates the reservation events and we merely publish them here
    mod->signalReservationEvent.connect([this](types::evse_manager::SessionEvent j) {
        j.uuid.clear(); // Reservation is not part of a session
        publish_session_event(j);
    });

    mod->charger->signal_session_started_event.connect(
        [this](const types::evse_manager::StartSessionReason& start_reason,
               const std::optional<types::authorization::ProvidedIdToken>& provided_id_token) {
            types::evse_manager::SessionEvent se;
            se.event = types::evse_manager::SessionEventEnum::SessionStarted;
            this->mod->selected_protocol = "IEC61851-1";
            types::evse_manager::SessionStarted session_started;

            se.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
            session_started.meter_value = mod->get_latest_powermeter_data_billing();

            if (mod->config.disable_authentication &&
                start_reason == types::evse_manager::StartSessionReason::EVConnected) {

                // Free service, authorize immediately in a separate thread (to avoid dead lock with charger state
                // machine, this signal handler runs in state machine context)
                std::thread authorize_thread([this]() {
                    types::authorization::ProvidedIdToken provided_token;
                    provided_token.authorization_type = types::authorization::AuthorizationType::RFID;
                    provided_token.id_token = {"FREESERVICE", types::authorization::IdTokenType::Local};
                    provided_token.prevalidated = true;
                    mod->charger->authorize(true, provided_token,
                                            {types::authorization::AuthorizationStatus::Accepted});
                    mod->charger_was_authorized();
                });
                authorize_thread.detach();
            }

            session_started.reason = start_reason;
            const auto session_uuid = this->mod->charger->get_session_id();
            session_started.meter_value = mod->get_latest_powermeter_data_billing();
            session_started.id_tag = provided_id_token;
            if (mod->is_reserved()) {
                session_started.reservation_id = mod->get_reservation_id();
                if (start_reason == types::evse_manager::StartSessionReason::Authorized) {
                    this->mod->cancel_reservation(false);
                }
            }

            session_started.logging_path = session_log.startSession(
                mod->config.logfile_suffix == "session_uuid" ? session_uuid : mod->config.logfile_suffix);

            session_log.evse(false, fmt::format("Session Started: {}",
                                                types::evse_manager::start_session_reason_to_string(start_reason)));

            mod->telemetry.publish("session", "events",
                                   {
                                       {"timestamp", Everest::Date::to_rfc3339(date::utc_clock::now())},
                                       {"type", "session_started"},
                                       {"session_id", session_uuid},
                                       {"reason", types::evse_manager::start_session_reason_to_string(start_reason)},
                                   });

            se.session_started = session_started;
            se.uuid = session_uuid;
            publish_session_event(se);
        });

    mod->charger->signal_transaction_started_event.connect([this](
                                                               const types::authorization::ProvidedIdToken& id_token) {
        types::evse_manager::SessionEvent se;
        se.event = types::evse_manager::SessionEventEnum::TransactionStarted;
        types::evse_manager::TransactionStarted transaction_started;
        se.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());

        transaction_started.meter_value = mod->get_latest_powermeter_data_billing();
        if (mod->is_reserved()) {
            transaction_started.reservation_id.emplace(mod->get_reservation_id());
            mod->cancel_reservation(false); // this allows OCPP1.6 to not move back to available.
        }

        transaction_started.id_tag = id_token;

        double energy_import = transaction_started.meter_value.energy_Wh_import.total;

        session_log.evse(false, fmt::format("Transaction Started ({} kWh)", energy_import / 1000.));
        const auto session_uuid = this->mod->charger->get_session_id();

        Everest::TelemetryMap telemetry_data = {
            {"timestamp", Everest::Date::to_rfc3339(date::utc_clock::now())},
            {"type", "transaction_started"},
            {"session_id", session_uuid},
            {"energy_counter_import_wh", transaction_started.meter_value.energy_Wh_import.total},
            {"id_tag", transaction_started.id_tag.id_token.value}};

        if (transaction_started.meter_value.energy_Wh_export.has_value()) {
            telemetry_data["energy_counter_export_wh"] = transaction_started.meter_value.energy_Wh_export.value().total;
        }
        mod->telemetry.publish("session", "events", telemetry_data);

        se.transaction_started.emplace(transaction_started);
        se.uuid = session_uuid;
        publish_session_event(se);
    });

    mod->charger->signal_transaction_finished_event.connect(
        [this](const types::evse_manager::StopTransactionReason& finished_reason,
               std::optional<types::authorization::ProvidedIdToken> finish_token) {
            types::evse_manager::SessionEvent se;

            se.event = types::evse_manager::SessionEventEnum::TransactionFinished;
            this->mod->selected_protocol = "Unknown";
            types::evse_manager::TransactionFinished transaction_finished;

            se.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());

            transaction_finished.meter_value = mod->get_latest_powermeter_data_billing();

            transaction_finished.reason.emplace(finished_reason);
            transaction_finished.id_tag = finish_token;

            double energy_import = transaction_finished.meter_value.energy_Wh_import.total;

            session_log.evse(false, fmt::format("Transaction Finished: {} ({} kWh)",
                                                types::evse_manager::stop_transaction_reason_to_string(finished_reason),
                                                energy_import / 1000.));
            const auto session_uuid = this->mod->charger->get_session_id();
            Everest::TelemetryMap telemetry_data = {
                {"timestamp", Everest::Date::to_rfc3339(date::utc_clock::now())},
                {"type", "transaction_finished"},
                {"session_id", session_uuid},
                {"energy_counter_import_wh", energy_import},
                {"reason", types::evse_manager::stop_transaction_reason_to_string(finished_reason)}};

            if (transaction_finished.meter_value.energy_Wh_export.has_value()) {
                telemetry_data["energy_counter_export_wh"] =
                    transaction_finished.meter_value.energy_Wh_export.value().total;
            }

            transaction_finished.start_signed_meter_value = mod->charger->get_start_signed_meter_value();
            transaction_finished.signed_meter_value = mod->charger->get_stop_signed_meter_value();
            mod->telemetry.publish("session", "events", telemetry_data);

            se.transaction_finished.emplace(transaction_finished);
            se.uuid = session_uuid;

            publish_session_event(se);
        });

    mod->charger->signal_simple_event.connect([this](const types::evse_manager::SessionEventEnum& e) {
        types::evse_manager::SessionEvent se;

        se.event = e;
        se.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());

        const auto session_uuid = this->mod->charger->get_session_id();
        if (e == types::evse_manager::SessionEventEnum::SessionFinished) {
            types::evse_manager::SessionFinished session_finished;
            session_finished.meter_value = mod->get_latest_powermeter_data_billing();
            se.session_finished = session_finished;
            session_log.evse(false, fmt::format("Session Finished"));
            session_log.stopSession();
            mod->telemetry.publish("session", "events",
                                   {{"timestamp", Everest::Date::to_rfc3339(date::utc_clock::now())},
                                    {"type", "session_finished"},
                                    {"session_id", session_uuid}});
        } else if (e == types::evse_manager::SessionEventEnum::Enabled or
                   e == types::evse_manager::SessionEventEnum::Disabled) {
            if (connector_status_changed) {
                se.connector_id = 1;
            }

            // Add source information (Who initiated this state change)
            se.source = mod->charger->get_last_enable_disable_source();
        } else if (e == types::evse_manager::SessionEventEnum::ChargingPausedEV or
                   e == types::evse_manager::SessionEventEnum::ChargingPausedEVSE or
                   e == types::evse_manager::SessionEventEnum::ChargingStarted or
                   e == types::evse_manager::SessionEventEnum::ChargingResumed) {
            types::evse_manager::ChargingStateChangedEvent charging_state_changed_event;
            charging_state_changed_event.meter_value = mod->get_latest_powermeter_data_billing();
            se.charging_state_changed_event = charging_state_changed_event;
        } else if (e == types::evse_manager::SessionEventEnum::Authorized or
                   e == types::evse_manager::SessionEventEnum::Deauthorized) {
            types::evse_manager::AuthorizationEvent authorization_event;
            authorization_event.meter_value = mod->get_latest_powermeter_data_billing();
            se.authorization_event = authorization_event;
        }

        se.uuid = session_uuid;

        publish_session_event(se);

        if (e == types::evse_manager::SessionEventEnum::SessionFinished) {
            this->mod->selected_protocol = "Unknown";
        }

        // Cancel reservations if charger is disabled
        if (mod->is_reserved() and e == types::evse_manager::SessionEventEnum::Disabled) {
            mod->cancel_reservation(true);
        }

        publish_selected_protocol(this->mod->selected_protocol);
    });

    mod->charger->signal_session_resumed_event.connect([this](const std::string& session_id) {
        types::evse_manager::SessionEvent session_event;
        session_event.uuid = session_id;
        session_event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
        session_event.event = types::evse_manager::SessionEventEnum::SessionResumed;
        publish_session_event(session_event);
    });

    // Note: Deprecated. Only kept for Node red compatibility, will be removed in the future
    // Legacy external mqtt pubs
    mod->charger->signal_max_current.connect([this](float c) {
        mod->mqtt.publish(fmt::format("everest_external/nodered/{}/state/max_current", mod->config.connector_id), c);

        limits.uuid = mod->info.id;
        limits.max_current = c;
        publish_limits(limits);
    });

    mod->charger->signal_state.connect([this](Charger::EvseState s) {
        mod->mqtt.publish(fmt::format("everest_external/nodered/{}/state/state_string", mod->config.connector_id),
                          mod->charger->evse_state_to_string(s));
        mod->mqtt.publish(fmt::format("everest_external/nodered/{}/state/state", mod->config.connector_id),
                          static_cast<int>(s));
    });
}

types::evse_manager::Evse evse_managerImpl::handle_get_evse() {
    types::evse_manager::Evse evse;
    evse.id = this->mod->config.connector_id;

    std::vector<types::evse_manager::Connector> connectors;
    types::evse_manager::Connector connector;
    // EvseManager currently only supports a single connector with id: 1;
    connector.id = 1;
    if (!this->mod->config.connector_type.empty()) {
        try {
            connector.type = types::evse_manager::string_to_connector_type_enum(this->mod->config.connector_type);
        } catch (const std::out_of_range& e) {
            EVLOG_warning << "Evse with id " << evse.id << ": connector type invalid: " << e.what();
        }
    }

    connectors.push_back(connector);
    evse.connectors = connectors;
    return evse;
}

bool evse_managerImpl::handle_enable_disable(int& connector_id, types::evse_manager::EnableDisableSource& cmd_source) {
    connector_status_changed = connector_id != 0;
    return mod->charger->enable_disable(connector_id, cmd_source);
};

void evse_managerImpl::handle_authorize_response(types::authorization::ProvidedIdToken& provided_token,
                                                 types::authorization::ValidationResult& validation_result) {
    const auto pnc = provided_token.authorization_type == types::authorization::AuthorizationType::PlugAndCharge;

    if (validation_result.authorization_status == types::authorization::AuthorizationStatus::Accepted) {

        if (this->mod->get_hlc_waiting_for_auth_pnc() && !pnc) {
            EVLOG_info
                << "EvseManager received Authorization other than PnC while waiting for PnC. This has no effect.";
            return;
        }

        this->mod->charger->authorize(true, provided_token, validation_result);
        mod->charger_was_authorized();
        if (validation_result.reservation_id.has_value()) {
            EVLOG_debug << "Reserve evse manager reservation id for id " << validation_result.reservation_id.value();
            // The validation result returns a reservation id. If this was a reservation for a specific evse, the
            // evse manager probably already stored the reservation id (and this call is not really necessary). But if
            // the reservation was not for a specific evse, the evse manager still has to send the reservation id in the
            // transaction event request. So that is why we call 'reserve' here, so the evse manager knows the
            // reservation id that belongs to this specific session and can send it accordingly.
            // As this is not a new reservation but an existing one, we don't signal a reservation event for this.
            mod->reserve(validation_result.reservation_id.value(), false);
        }
    }

    if (pnc) {
        this->mod->r_hlc[0]->call_authorization_response(
            validation_result.authorization_status,
            validation_result.certificate_status.value_or(types::authorization::CertificateStatus::Accepted));
    }
};

void evse_managerImpl::handle_withdraw_authorization() {
    // reservation_id might have been stored when reserved id token has been authorized, but timed out, so
    //  we can consider the reservation as consumed
    if (mod->charger->get_authorized_eim() and mod->is_reserved()) {
        mod->cancel_reservation(true);
    }
    this->mod->charger->deauthorize();
};

bool evse_managerImpl::handle_reserve(int& reservation_id) {
    return mod->reserve(reservation_id, true);
};

void evse_managerImpl::handle_cancel_reservation() {
    mod->cancel_reservation(true);
};

void evse_managerImpl::handle_set_faulted() {
    mod->charger->set_faulted();
};

bool evse_managerImpl::handle_pause_charging() {
    return mod->charger->pause_charging();
};

bool evse_managerImpl::handle_resume_charging() {
    return mod->charger->resume_charging();
};

bool evse_managerImpl::handle_stop_transaction(types::evse_manager::StopTransactionRequest& request) {
    return mod->charger->cancel_transaction(request);
};

bool evse_managerImpl::handle_external_ready_to_start_charging() {
    if (mod->config.external_ready_to_start_charging) {
        EVLOG_info << "Received external ready to start charging command.";
        mod->ready_to_start_charging();
        return true;
    } else {
        EVLOG_warning
            << "Ignoring external ready to start charging command, this could be a configuration issue. Please check "
               "if 'external_ready_to_start_charging' is set to true if you want to use this feature.";
    }

    return false;
}

bool evse_managerImpl::handle_force_unlock(int& connector_id) {
    if (not mod->r_connector_lock.empty()) {
        types::evse_manager::StopTransactionRequest request;
        request.reason = types::evse_manager::StopTransactionReason::UnlockCommand;
        mod->charger->cancel_transaction(request);
        mod->bsp->connector_force_unlock();
        return true;
    }
    return false;
};

void evse_managerImpl::handle_set_plug_and_charge_configuration(
    types::evse_manager::PlugAndChargeConfiguration& plug_and_charge_configuration) {
    if (plug_and_charge_configuration.pnc_enabled.has_value()) {
        mod->set_pnc_enabled(plug_and_charge_configuration.pnc_enabled.value());
    }
    if (plug_and_charge_configuration.central_contract_validation_allowed.has_value()) {
        mod->set_central_contract_validation_allowed(
            plug_and_charge_configuration.central_contract_validation_allowed.value());
    }
    if (plug_and_charge_configuration.contract_certificate_installation_enabled.has_value()) {
        mod->set_contract_certificate_installation_enabled(
            plug_and_charge_configuration.contract_certificate_installation_enabled.value());
    }
}

types::evse_manager::UpdateAllowedEnergyTransferModesResult
evse_managerImpl::handle_update_allowed_energy_transfer_modes(
    std::vector<types::iso15118::EnergyTransferMode>& allowed_energy_transfer_modes) {
    // TODO(mlitre): Add check for incompatible type
    if (!mod->r_hlc.empty() && mod->r_hlc[0]) {
        mod->r_hlc[0]->call_update_energy_transfer_modes(allowed_energy_transfer_modes);
        return types::evse_manager::UpdateAllowedEnergyTransferModesResult::Accepted;
    }
    return types::evse_manager::UpdateAllowedEnergyTransferModesResult::NoHlc;
}

} // namespace evse
} // namespace module
