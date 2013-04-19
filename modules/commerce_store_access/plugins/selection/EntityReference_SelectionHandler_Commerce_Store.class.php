<?php

/**
 * Commerce Store entity selection handler.
 * This should only be used when referencing Commerce Store entities.
 */
class EntityReference_SelectionHandler_Commerce_Store implements EntityReference_SelectionHandler {

  protected function __construct($field, $instance = NULL, $entity_type = NULL, $entity = NULL) {
    $this->field = $field;
    $this->instance = $instance;
    $this->entity_type = $entity_type;
    $this->entity = $entity;
  }

  /**
   * Implements EntityReferenceHandler::getInstance().
   */
  public static function getInstance($field, $instance = NULL, $entity_type = NULL, $entity = NULL) {
    $target_entity_type = $field['settings']['target_type'];
    if ($target_entity_type != 'commerce_store') {
      return EntityReference_SelectionHandler_Broken::getInstance($field, $instance);
    }

    // Check if the entity type does exist and has a base table.
    $entity_info = entity_get_info($target_entity_type);
    if (empty($entity_info['base table'])) {
      return EntityReference_SelectionHandler_Broken::getInstance($field, $instance);
    }

    return new EntityReference_SelectionHandler_Commerce_Store($field, $instance, $entity_type, $entity);
  }


  /**
   * Implements EntityReferenceHandler::settingsForm().
   */
  public static function settingsForm($field, $instance) {
    return array();
  }

  /**
   * Implements EntityReferenceHandler::getReferencableEntities().
   */
  public function getReferencableEntities($match = NULL, $match_operator = 'CONTAINS', $limit = 0) {
    $options = array();

    $query = $this->buildEntityFieldQuery($match, $match_operator);
    if ($limit > 0) {
      $query->range(0, $limit);
    }

    $results = $query->execute();
    if (!empty($results['commerce_store'])) {
      $entity_keys = array_keys($results['commerce_store']);
      $entities = entity_load('commerce_store', $entity_keys);
      foreach ($entities as $entity_id => $entity) {
        if (commerce_store_access_user_access('add products to the store', $entity)) {
          $options[$entity_id] = array($entity_id => check_plain($this->getLabel($entity)));
        }
      }
    }


    return $options;
  }

  /**
   * Implements EntityReferenceHandler::countReferencableEntities().
   */
  public function countReferencableEntities($match = NULL, $match_operator = 'CONTAINS') {
    $query = $this->buildEntityFieldQuery($match, $match_operator);
    $results = $query->execute();
    $count = 0;
    if (!empty($results['commerce_store'])) {
      $entity_keys = array_keys($results['commerce_store']);
      $entities = entity_load('commerce_store', $entity_keys);
      foreach ($entities as $entity_id => $entity) {
        if (commerce_store_access_user_access('add products to the store', $entity)) {
          $count++;
        }
      }
    }
    return $count;
  }

  /**
   * Implements EntityReferenceHandler::validateReferencableEntities().
   */
  public function validateReferencableEntities(array $ids) {
    if ($ids) {
      $entity_type = $this->field['settings']['target_type'];
      $query = $this->buildEntityFieldQuery();
      $query->entityCondition('entity_id', $ids, 'IN');
      $result = $query->execute();
      if (!empty($result[$entity_type])) {
        $entities = $result[$entity_type];
        $valid_ids = array();
        foreach (array_keys($entities) as $entity_id) {
          if (commerce_store_access_user_access('add products to the store', commerce_store_load($entity_id))) {
            $valid_ids[] = $entity_id;
          }
        }
        return $valid_ids;
      }
    }

    return array();
  }

  /**
   * Implements EntityReferenceHandler::validateAutocompleteInput().
   */
  public function validateAutocompleteInput($input, &$element, &$form_state, $form) {
      $entities = $this->getReferencableEntities($input, '=', 6);
      if (empty($entities)) {
        // Error if there are no entities available for a required field.
        form_error($element, t('There are no entities matching "%value"', array('%value' => $input)));
      }
      elseif (count($entities) > 5) {
        // Error if there are more than 5 matching entities.
        form_error($element, t('Many entities are called %value. Specify the one you want by appending the id in parentheses, like "@value (@id)"', array(
          '%value' => $input,
          '@value' => $input,
          '@id' => key($entities),
        )));
      }
      elseif (count($entities) > 1) {
        // More helpful error if there are only a few matching entities.
        $multiples = array();
        foreach ($entities as $id => $name) {
          $multiples[] = $name . ' (' . $id . ')';
        }
        form_error($element, t('Multiple entities match this reference; "%multiple"', array('%multiple' => implode('", "', $multiples))));
      }
      else {
        // Take the one and only matching entity.
        return key($entities);
      }
  }

  /**
   * Build an EntityFieldQuery to get referencable entities.
   */
  protected function buildEntityFieldQuery($match = NULL, $match_operator = 'CONTAINS') {
    $query = new EntityFieldQuery();
    global $user;
    $query->entityCondition('entity_type', 'commerce_store');
    $query->fieldCondition('cmp_m_store', 'target_id', $user->uid);
    if (isset($match)) {
      $entity_info = entity_get_info('commerce_store');
      if (isset($entity_info['entity keys']['label'])) {
        $query->propertyCondition($entity_info['entity keys']['label'], $match, $match_operator);
      }
    }

    // Add a generic entity access tag to the query.
    $query->addTag($this->field['settings']['target_type'] . '_access');
    $query->addTag('entityreference');
    $query->addMetaData('field', $this->field);
    $query->addMetaData('entityreference_selection_handler', $this);


    return $query;
  }

  /**
   * Implements EntityReferenceHandler::entityFieldQueryAlter().
   */
  public function entityFieldQueryAlter(SelectQueryInterface $query) {

  }

  /**
   * Helper method: pass a query to the alteration system again.
   *
   * This allow Entity Reference to add a tag to an existing query, to ask
   * access control mechanisms to alter it again.
   */
  protected function reAlterQuery(SelectQueryInterface $query, $tag, $base_table) {
    // Save the old tags and metadata.
    // For some reason, those are public.
    $old_tags = $query->alterTags;
    $old_metadata = $query->alterMetaData;

    $query->alterTags = array($tag => TRUE);
    $query->alterMetaData['base_table'] = $base_table;
    drupal_alter(array('query', 'query_' . $tag), $query);

    // Restore the tags and metadata.
    $query->alterTags = $old_tags;
    $query->alterMetaData = $old_metadata;
  }

  /**
   * Implements EntityReferenceHandler::getLabel().
   */
  public function getLabel($entity) {
    return entity_label($this->entity_type, $entity);
  }
}

